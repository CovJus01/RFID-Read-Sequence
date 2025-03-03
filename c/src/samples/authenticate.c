/**
* Sample program that to demonstrate the usage of Gen2v2 Authenticate.
* @file authenticate.c
*/

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>


#if WIN32
#define snprintf sprintf_s
#endif 

/* Enable this to use transportListener */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 0 
#endif

#define usage() {errx(1, "Please provide valid reader URL, such as: reader-uri [--ant n]\n"\
                         "reader-uri : e.g., 'tmr:///COM1' or 'tmr:///dev/ttyS0/' or 'tmr://readerIP'\n"\
                         "[--ant n] : e.g., '--ant 1'\n"\
                         "Example: 'tmr:///com4' or 'tmr:///com4 --ant 1,2' \n");}

void errx(int exitval, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);

	exit(exitval);
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
	if (TMR_SUCCESS != ret)
	{
		errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
	}
}

void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
									 uint32_t timeout, void *cookie)
{
	FILE *out = cookie;
	uint32_t i;

	fprintf(out, "%s", tx ? "Sending: " : "Received:");
	for (i = 0; i < dataLen; i++)
	{
		if (i > 0 && (i & 15) == 0)
		{
			fprintf(out, "\n         ");
		}
		fprintf(out, " %02x", data[i]);
	}
	fprintf(out, "\n");
}

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
	FILE *out = cookie;

	fprintf(out, "%s", tx ? "Sending: " : "Received:");
	fprintf(out, "%s\n", data);
}

void parseAntennaList(uint8_t *antenna, uint8_t *antennaCount, char *args)
{
	char *token = NULL;
	char *str = ",";
	uint8_t i = 0x00;
	int scans;

	/* get the first token */
	if (NULL == args)
	{
		fprintf(stdout, "Missing argument\n");
		usage();
	}

	token = strtok(args, str);
	if (NULL == token)
	{
		fprintf(stdout, "Missing argument after %s\n", args);
		usage();
	}

	while(NULL != token)
	{
#ifdef WIN32
		scans = sscanf(token, "%hh"SCNu8, &antenna[i]);
#else
		scans = sscanf(token, "%"SCNu8, &antenna[i]);
#endif
		if (1 != scans)
		{
			fprintf(stdout, "Can't parse '%s' as an 8-bit unsigned integer value\n", token);
			usage();
		}
		i++;
		token = strtok(NULL, str);
	}
	*antennaCount = i;
}
#ifdef TMR_ENABLE_UHF
/* performEmbeddedTagOperation() performs following functions:
 * a) Set the tagOp to the read plan.
 * b) Set the filter to the read plan
 * c) set the read plan to the reader.
 * d) perform sync read.
 * e) process the data if it is there.
 * 
 * @param rp    : pointer to the reader object.
 * @param plan  : pointer to the read plan object
 * @param tagOp : pointer to the tag operation object
 * @param filter: pointer to the filter object 
*/
void performEmbeddedTagOperation(TMR_Reader *reader, TMR_ReadPlan *plan, TMR_TagOp *tagOp, TMR_TagFilter *filter)
{
  TMR_Status ret;

  // Set tagOp to read plan 
  ret = TMR_RP_set_tagop(plan, tagOp);
  checkerr(reader, ret, 1, "setting tagop");

  // Set filter to read plan
  ret = TMR_RP_set_filter(plan,filter);
  checkerr(reader, ret, 1, "setting filter");

  // Commit read plan 
  ret = TMR_paramSet(reader, TMR_PARAM_READ_PLAN, plan);
  checkerr(reader, ret, 1, "setting read plan");

  ret = TMR_read(reader, 500, NULL);
  if (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)
  {
    /* In case of TAG ID Buffer Full, extract the tags present
     * in buffer.
    */
    fprintf(stdout, "reading tags:%s\n", TMR_strerr(reader, ret));
  }
  else
  {
    checkerr(reader, ret, 1, "reading tags");
  }  

  while (TMR_SUCCESS == TMR_hasMoreTags(reader))
  {
    TMR_TagReadData trd;
    uint8_t dataBuf[255];
    char epcStr[128];

    ret = TMR_TRD_init_data(&trd, sizeof(dataBuf)/sizeof(uint8_t), dataBuf);
    checkerr(reader, ret, 1, "creating tag read data");

    ret = TMR_getNextTag(reader, &trd);
    checkerr(reader, ret, 1, "fetching tag");
    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcStr);
    printf("EPC : %s\n", epcStr);
    if(0 < trd.data.len)
    {
      char dataStr[255];
      uint8_t iChallenge[255];

      memmove(iChallenge, trd.data.list, 10);
      TMR_bytesToHex(iChallenge, 10, dataStr);
      printf("Generated Ichallenge: %s\n", dataStr);

      TMR_bytesToHex(trd.data.list + 10, trd.data.len - 10, dataStr);
      printf("Raw data(%d): %s\n", (trd.data.len - 10), dataStr);
    }
  }
}
#endif /* TMR_ENABLE_UHF */
int main(int argc, char *argv[])
{
	TMR_Reader r, *rp;
	TMR_Status ret;
#ifdef TMR_ENABLE_UHF
	TMR_Region region;
#endif /* TMR_ENABLE_UHF */
	uint8_t *antennaList = NULL;
	uint8_t buffer[20];
	uint8_t i;
	uint8_t antennaCount = 0x0;
#if USE_TRANSPORT_LISTENER
	TMR_TransportListenerBlock tb;
#endif

	if (argc < 2)
	{
		usage();
	}

	for (i = 2; i < argc; i+=2)
	{
		if(0x00 == strcmp("--ant", argv[i]))
		{
			if (NULL != antennaList)
			{
				fprintf(stdout, "Duplicate argument: --ant specified more than once\n");
				usage();
			}
			parseAntennaList(buffer, &antennaCount, argv[i+1]);
			antennaList = buffer;
		}
		else
		{
			fprintf(stdout, "Argument %s is not recognized\n", argv[i]);
			usage();
		}
	}

	rp = &r;
	ret = TMR_create(rp, argv[1]);
	checkerr(rp, ret, 1, "creating reader");

#if USE_TRANSPORT_LISTENER

	if (TMR_READER_TYPE_SERIAL == rp->readerType)
	{
		tb.listener = serialPrinter;
	}
	else
	{
		tb.listener = stringPrinter;
	}
	tb.cookie = stdout;

	TMR_addTransportListener(rp, &tb);
#endif

	ret = TMR_connect(rp);
	checkerr(rp, ret, 1, "connecting reader");

#ifdef TMR_ENABLE_UHF
	region = TMR_REGION_NONE;
	ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
	checkerr(rp, ret, 1, "getting region");

	if (TMR_REGION_NONE == region)
	{
		TMR_RegionList regions;
		TMR_Region _regionStore[32];
		regions.list = _regionStore;
		regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
		regions.len = 0;

		ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
		checkerr(rp, ret, __LINE__, "getting supported regions");

		if (regions.len < 1)
		{
			checkerr(rp, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't supportany regions");
		}
		region = regions.list[0];
		ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
		checkerr(rp, ret, 1, "setting region");  
	}

  //Use first antenna for operation
  if (NULL != antennaList)
  {
    ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &antennaList[0]);
    checkerr(rp, ret, 1, "setting tagop antenna");  
  }

	/* Read Plan */
	{
		TMR_ReadPlan plan;
		/**
		* for antenna configuration we need two parameters
		* 1. antennaCount : specifies the no of antennas should
		*    be included in the read plan, out of the provided antenna list.
		* 2. antennaList  : specifies  a list of antennas for the read plan.
		**/ 

		// initialize the read plan 
		ret = TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);
		checkerr(rp, ret, 1, "initializing the  read plan");

		/* Commit read plan */
		ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
		checkerr(rp, ret, 1, "setting read plan");

		{

			TMR_TagOp newtagop;
			//TMR_TagFilter selectFilter;
			TMR_uint8List key;
			bool SendRawData = false;
			bool _isNMV2DTag = false;
            bool enableEmbeddedTagop = false;
			uint8_t Offset = 0;
			uint8_t BlockCount = 1;
			TMR_TagOp_GEN2_NXP_Tam1Authentication tam1;
			TMR_TagOp_GEN2_NXP_Tam2Authentication tam2;
			TMR_TagOp_GEN2_NXP_Authenticate authenticate;
			uint8_t data[128];
			TMR_uint8List dataList;
			int protMode;
			//uint8_t mask[128];
			uint8_t key0[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
			//uint8_t key1[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

			dataList.len = dataList.max = 128;
			dataList.list = data;

			/* mask[0] = 0xE2;
			mask[1] = 0xC0;
			mask[2] = 0x6F;
			mask[3] = 0x92;
			TMR_TF_init_gen2_select(&selectFilter, false, TMR_GEN2_BANK_EPC, 32, 16, mask);*/

			// Uncomment this to enable Authenticate with TAM1 with key0 for NXPUCODE AES tag
            key.list = key0;
            key.max = key.len = sizeof(key0) / sizeof(key0[0]);
            ret = TMR_TagOp_init_GEN2_NXP_AES_Tam1authentication(&tam1, KEY0, &key, SendRawData);
			checkerr(rp, ret, 1, "initializing Tam1 authentication");
			authenticate.tam1Auth = tam1;
			authenticate.type = TAM1_AUTHENTICATION;

			ret = TMR_TagOp_init_GEN2_NXP_AES_Authenticate(&newtagop,&authenticate);
			checkerr(rp, ret, 1, "initializing Authenticate");

			ret = TMR_executeTagOp(rp, &newtagop, NULL, &dataList);
			checkerr(rp, ret, 1, "executing Authenticate tagop");
			if(SendRawData)
			{
              //TODO:Currently C API don't have support for decrypting raw data.
              // TO decrypt this data use online AES decryptor
              //http://aes.online-domain-tools.com/
              char dataStr[255];
              uint8_t iChallenge[255];

              memmove(iChallenge, dataList.list, 10);
              TMR_bytesToHex(iChallenge, 10, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
  
              TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
              printf("Raw data :%s\n", dataStr);
			}
            else
            {
              char dataStr[255];
              TMR_bytesToHex(dataList.list ,dataList.len, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
            }

            // Enable embedded tagOp operation by making enableEmbeddedTagop = true 
            if(enableEmbeddedTagop)
              performEmbeddedTagOperation(rp, &plan, &newtagop, NULL);

			//Uncomment this to enable Authenticate with TAM1 with key1
			/*key.list = key1;
			key.max = key.len = sizeof(key1) / sizeof(key1[0]);
			ret = TMR_TagOp_init_GEN2_NXP_AES_Tam1authentication(&tam1, KEY1, &key, SendRawData);
			checkerr(rp, ret, 1, "initializing Tam1 authentication");

			authenticate.tam1Auth = tam1;
			authenticate.type = TAM1_AUTHENTICATION;

			ret = TMR_TagOp_init_GEN2_NXP_AES_Authenticate(&newtagop,&authenticate);
			checkerr(rp, ret, 1, "initializing Authenticate");

			ret = TMR_executeTagOp(rp, &newtagop, NULL, &dataList);
			checkerr(rp, ret, 1, "executing Authenticate tagop");
			if(SendRawData)
			{
              //TODO:Currently C API don't have support for decrypting raw data.
              // TO decrypt this data use online AES decryptor
              //http://aes.online-domain-tools.com/
              char dataStr[255];
              uint8_t iChallenge[255];

              memmove(iChallenge, dataList.list, 10);
              TMR_bytesToHex(iChallenge, 10, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
  
              TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
              printf("Raw data: %s\n", dataStr);
			}
            else
            {
              char dataStr[255];
              TMR_bytesToHex(dataList.list ,dataList.len, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
            }

            // Enable embedded tagOp operation by making enableEmbeddedTagop = true 
            if(enableEmbeddedTagop)
              performEmbeddedTagOperation(rp, &plan, &newtagop, NULL);*/

			//Uncomment this to enable Authenticate with TAM2 with key1

			/*Note- TMR_NXP_Profile name is changed from 'EPC' to 'TMR_NXP_Profile_EPC'
			 * as part of Bug#8976 fix (Because of the conflicting macro name "EPC".
			 * It's the name of an ESP32 register, so we shouldn't use it without a unique prefix).
			 */

			/*key.list = key1;
			key.max = key.len = sizeof(key1) / sizeof(key1[0]);

			// supported protMode value is 1 for NXPUCODE AES
			 protMode = 1;
			ret = TMR_TagOp_init_GEN2_NXP_AES_Tam2authentication(&tam2, KEY1, &key, TMR_NXP_Profile_EPC, Offset, BlockCount, protMode, SendRawData);
			checkerr(rp, ret, 1, "initializing Tam1 authentication");

			authenticate.type = TAM2_AUTHENTICATION;
			authenticate.tam2Auth = tam2;

			ret = TMR_TagOp_init_GEN2_NXP_AES_Authenticate(&newtagop,&authenticate);
			checkerr(rp, ret, 1, "initializing Authenticate");

			ret = TMR_executeTagOp(rp, &newtagop, NULL, &dataList);
			checkerr(rp, ret, 1, "executing Authenticate tagop");
			if(SendRawData)
			{
              //TODO:Currently C API don't have support for decrypting raw data.
              // TO decrypt this data use online AES decryptor
              //http://aes.online-domain-tools.com/
              char dataStr[255];
              uint8_t iChallenge[255];

              memmove(iChallenge, dataList.list, 10);
              TMR_bytesToHex(iChallenge, 10, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
  
              TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
              printf("Raw data: %s\n", dataStr);
			}
            else
            {
              char dataStr[255];
              uint8_t iChallenge[255];

              memmove(iChallenge, dataList.list, 10);
              TMR_bytesToHex(iChallenge, 10, dataStr);
              printf("Generated Ichallenge: %s\n", dataStr);
  
              TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
              printf("data: %s\n", dataStr);
            }

            // Enable embedded tagOp operation by making enableEmbeddedTagop = true 
            if(enableEmbeddedTagop)
              performEmbeddedTagOperation(rp, &plan, &newtagop, NULL);*/

			//Enable flag _isNMV2DTag for TAM1/TAM2 Authentication with KEY0 for NMV2D Tag
			if (_isNMV2DTag) 
			{
				// NMV2D tag only supports KEY0
				uint8_t key0_NMV2D[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				
				key.list = key0_NMV2D;
				key.max = key.len = sizeof(key0_NMV2D) / sizeof(key0_NMV2D[0]);

				//Uncomment this to enable Authenticate with TAM1 with key0
				
				/*ret = TMR_TagOp_init_GEN2_NXP_AES_Tam1authentication(&tam1, KEY0, &key, SendRawData);
				checkerr(rp, ret, 1, "initializing Tam1 authentication");

				authenticate.tam1Auth = tam1;
				authenticate.type = TAM1_AUTHENTICATION;

				ret = TMR_TagOp_init_GEN2_NXP_AES_Authenticate(&newtagop,&authenticate);
				checkerr(rp, ret, 1, "initializing Authenticate");

				ret = TMR_executeTagOp(rp, &newtagop, NULL, &dataList);
				checkerr(rp, ret, 1, "executing Authenticate tagop");
				if(SendRawData)
				{
                  //TODO:Currently C API don't have support for decrypting raw data.
                  // TO decrypt this data use online AES decryptor
                  //http://aes.online-domain-tools.com/
                  char dataStr[255];
                  uint8_t iChallenge[255];

                  memmove(iChallenge, dataList.list, 10);
                  TMR_bytesToHex(iChallenge, 10, dataStr);
                  printf("Generated Ichallenge: %s\n", dataStr);
  
                  TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
                  printf("Raw data: %s\n", dataStr);
				}
                else
                {
                  char dataStr[255];
                  TMR_bytesToHex(dataList.list ,dataList.len, dataStr);
                  printf("Generated Ichallenge: %s\n", dataStr);
                }*/

				// Authenticate with TAM2 with key0
				//supported protMode values are 0,1,2,3.

				/*Note- TMR_NXP_Profile name is changed from 'EPC' to 'TMR_NXP_Profile_EPC'
				 * as part of Bug#8976 fix (Because of the conflicting macro name "EPC".  
				 * It's the name of an ESP32 register, so we shouldn't use it without a unique prefix).
				 */
				protMode = 0;
				ret = TMR_TagOp_init_GEN2_NXP_AES_Tam2authentication(&tam2, KEY0, &key, TMR_NXP_Profile_EPC, Offset, BlockCount, protMode, SendRawData);
				checkerr(rp, ret, 1, "initializing Tam1 authentication");

				authenticate.type = TAM2_AUTHENTICATION;
				authenticate.tam2Auth = tam2;

				ret = TMR_TagOp_init_GEN2_NXP_AES_Authenticate(&newtagop,&authenticate);
				checkerr(rp, ret, 1, "initializing Authenticate");

				ret = TMR_executeTagOp(rp, &newtagop, NULL, &dataList);
				checkerr(rp, ret, 1, "executing Authenticate tagop");
				if(SendRawData)
				{
                  //TODO:Currently C API don't have support for decrypting raw data.
                  // TO decrypt this data use online AES decryptor
                  //http://aes.online-domain-tools.com/
                  char dataStr[255];
                  uint8_t iChallenge[255];

                  memmove(iChallenge, dataList.list, 10);
                  TMR_bytesToHex(iChallenge, 10, dataStr);
                  printf("Generated Ichallenge: %s\n", dataStr);
  
                  TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
                  printf("Raw data: %s\n", dataStr);;
				}
				else
				{
                  char dataStr[255];
                  uint8_t iChallenge[255];

                  memmove(iChallenge, dataList.list, 10);
                  TMR_bytesToHex(iChallenge, 10, dataStr);
                  printf("Generated Ichallenge: %s\n", dataStr);
  
                  TMR_bytesToHex(dataList.list + 10, dataList.len - 10, dataStr);
                  printf("Data: %s\n", dataStr);
				}
			}
		}
	}
#endif /*  TMR_ENABLE_UHF */
	TMR_destroy(rp);
	return 0;
}
