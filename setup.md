# Setup inscructions
## Steps

1. Clone repo
2. Install required dependencies
3. Download MercuryAPI
4. Setup MercuryAPI shared library

## Step 1 - Clone Repo

The repo for this project can be found here:

    https://github.com/CovJus01/RFID-Read-Sequence/tree/main

To clone the repo, perform the following

    git clone https://github.com/CovJus01/RFID-Read-Sequence/tree/main

The repo should now be present in a folder named "RFID-Read-Sequence"

## Step 2 - Install required dependencies

There are various dependencies required for this project that may not be installed
in your environment. The required dependencies are as follows:

- Readline
- GTK-3.0
- ldl
- lm
- lpthread

To install them on our system we need to install them using apt:

    sudo apt install libreadline libreadline-dev
    sudo apt install libc6-dev
    sudo apt install libm-dev
    sudo apt install libgtk-3-dev
    sudo apt install libgtk-3-0

Then all the required libraries should be installed

## Step 3 - Download MercuryAPI

MercuryAPI can be downloaded from the following webpage, it will come as a
compressed zip, extract it to a directory you'd like. We will need to access
it in the following step to create the shared library.

https://www.jadaktech.com/product/thingmagic-mercury-api/

## Step 4 - Setup the shared library

First we need to compile the library and source files to be packed for our shared
library. Navigate to the API source for C. The directory should look something like
this: "mercuryAPI/c/src/api"

Once in the library we will compile the files

    make TMR_ENABLE_SERIAL_READER_ONLY=1

Now to setup the shared library we do the following

    sudo bash
    mv libmercuryapi.so.1 /usr/lib
    ln -sf /usr/lib/libmercuryapi.so.1  /usr/lib/libmercuryapi.so.0
    ln -sf /usr/lib/libmercuryapi.so.1 /usr/lib/libmercuryapi.so

The library should now be fully setup and the project should compile effectively!


# Run Our program:

Head into the desired application and do the make command to run the application
