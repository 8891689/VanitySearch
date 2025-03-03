# VanitySearch
Forked from https://github.com/JeanLucPons/VanitySearch 

Please note that this is not my project. I just modified the private key, and it is displayed correctly, and the address found is also correct, but the last few digits of the address encoding are still wrong. Only the prefix address is correct, and the hexadecimal private key is correct.
```
VanitySeacrh [-check] [-v] [-u] [-b] [-c] [-gpu] [-stop] [-i inputfile]
             [-gpuId gpuId1[,gpuId2,...]] [-g g1x,g1y,[,g2x,g2y,...]]
             [-o outputfile] [-m maxFound] [-ps seed] [-s seed] [-t nbThread]
             [-start] [-bits] [-level]
             [-nosse] [-r rekey] [-check] [-kp] [-sp startPubKey]
             [-rp privkey partialkeyfile] [prefix]

 prefix: prefix to search (Can contains wildcard '?' or '*')
 -v: Print version
 -u: Search uncompressed addresses
 -b: Search both uncompressed or compressed addresses
 -c: Case unsensitive search
 -gpu: Enable gpu calculation
 -stop: Stop when all prefixes are found
 -i inputfile: Get list of prefixes to search from specified file
 -o outputfile: Output results to the specified file
 -gpu gpuId1,gpuId2,...: List of GPU(s) to use, default is 0
 -g g1x,g1y,g2x,g2y, ...: Specify GPU(s) kernel gridsize, default is 8*(MP number),128
 -m: Specify maximun number of prefixes found by each kernel call
 -s seed: Specify a seed for the base key, default is random
 -start startKey: Set Starting key, default is random bits 66
 -bits number: Set Random bits, default is random bits 66
 -level number: Set number 0-4 to Enable OpenSSL functions, default: -level 1
 -ps seed: Specify a seed concatened with a crypto secure random seed
 -t threadNumber: Specify number of CPU thread, default is number of core
 -nosse: Disable SSE hash function
 -l: List cuda enabled devices
 -check: Check CPU and GPU kernel vs CPU
 -cp privKey: Compute public key (privKey in hex hormat)
 -ca pubKey: Compute address (pubKey in hex hormat)
 -kp: Generate key pair
 -rp privkey partialkeyfile: Reconstruct final private key(s) from partial key(s) info.
 -sp startPubKey: Start the search with a pubKey (for private key splitting)
 -r rekey: Rekey interval in MegaKey, default is disabled
```

--------------------------------------------------------------------------------------------------------------
#CPU Vanity Address and Jigsaw Puzzle Instances

./VanitySearch -stop -t 4 -bits 28 -r 5000 -level 4 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 146150163733090291820368483271628301965593254297651899 
Search: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY [Compressed]
Start Mon Mar  3 08:38:34 2025
Base Key: Randomly changed every 5000 Mkeys
[1.81 Mkey/s][GPU 0.00 Mkey/s][Total 2^25.00][Prob 0.0%][50% in 1.77879e+34y][Found 0]
Warning, wrong private key generated !
  Addr :12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
  Check:12jbtzBb54r97TCwW3G1gCFoumpcmX5gH2
  Endo:0 incr:408 comp:1

./VanitySearch -stop -t 1 -bits 256 -r 500 -level 4 1btc
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 4553521 with CPU performance counter: 1740962636499159 
Search: 1btc [Compressed]
Start Mon Mar  3 08:43:56 2025
Base Key: Randomly changed every 500 Mkeys
[0.50 Mkey/s][GPU 0.00 Mkey/s][Total 2^22.74][Prob 78.4%][80% in 00:00:00][Found 0]EA38CBF60E

  Check:1btcUQyhULzuKzVxjERasEvXHEw9GenjJ

!!! Result.txt Found key: B8302B348CF3DA57BF4FC6DD9916DC051AC3018FAAB4952A673ECBEA393D7DA3 


----------------------------------------------------------------------------------------------------------------

./VanitySearch -t 0 -gpu -bits 68 -r 81689 -level 4 1MVDYgVa
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 888446610539 CPU performance counter: 1740964004863194 
Search: 1MVDYgVa [Compressed]
Start Mon Mar  3 09:06:44 2025
Base Key: Randomly changed every 81689 Mkeys
[1889.25 Mkey/s][GPU 1889.25 Mkey/s][Total 2^37.52][Prob 19.9%][50% in 00:03:41][Found 0]

  Check:1MVDYgVaUExtxfQE8u1HjZsaxFCDbuMyjZ

!!! Result.txt Found key: 903E84386E11BBD36 

[1880.26 Mkey/s][GPU 1880.26 Mkey/s][Total 2^38.40][Prob 33.6%][50% in 00:02:14][Found 1]

  Check:1MVDYgVaUEGCPPFaA7k47u71FtzjWTwNUh

!!! Result.txt Found key: EA8EFA548C0A75B15 

^C116.10 Mkey/s][GPU 2116.10 Mkey/s][Total 2^39.07][Prob 47.7%][50% in 00:00:18][Found 2]




---------------------------------------------------------------------------------------------------------------------
#GPU Vanity Address and Jigsaw Puzzle Instances
./VanitySearch -stop -t 0 -gpu -bits 38 -r 50000 -level 4 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 146150163733090291820368483271628301965593254297678069 
Search: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2 [Compressed]
Start Mon Mar  3 08:31:36 2025
Base Key: Randomly changed every 50000 Mkeys
[2231.46 Mkey/s][GPU 2231.46 Mkey/s][Total 2^34.38][Prob 0.0%][50% in 1.43956e+31y][Found 0]
Warning, wrong private key generated !
  Addr :1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
  Check:1HBtApAFA9B2YZw3G2YKSMCtb3dVkGnF4k
  Endo:0 incr:265 comp:1


./VanitySearch -stop -t 0 -gpu -bits 37 -r 100000 -level 4 14iXhn8bGajVWegZHJ18vJLHhntcpL4dex
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 146150163733090291820368483271628301965593254297696741 
Search: 14iXhn8bGajVWegZHJ18vJLHhntcpL4dex [Compressed]
Start Mon Mar  3 08:32:52 2025
Base Key: Randomly changed every 100000 Mkeys
[2352.67 Mkey/s][GPU 2352.67 Mkey/s][Total 2^33.13][Prob 0.0%][50% in 1.36539e+31y][Found 0]
Warning, wrong private key generated !
  Addr :14iXhn8bGajVWegZHJ18vJLHhntcpL4dex
  Check:14iXhn8bGajVWegZHJ18vJLHhntcnoS8tQ
  Endo:0 incr:483 comp:1

./VanitySearch -stop -t 0 -gpu -bits 256 -r 500000 -level 4 1Great
 
OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)
argv Add to[-start] [-bits]Cancel SSE 
[OpenSSL level 4]
Difficulty: 264104224ith CPU performance counter: 1740962534943094 
Search: 1Great [Compressed]
Start Mon Mar  3 08:42:14 2025
Base Key: Randomly changed every 500000 Mkeys
Bit 256 GPU Base Key 69631: A0D78D4B7874537F34578A864E28B040D860255575926F48D4B666591D760E38

  Check:1Great3ZB78x244SJ25Y6TZS85tfN6QsPm

!!! Result.txt Found key: FA3E108197F87188257B2CB9FB24FEEEF4E2C3E45EB3F08CE36579BF8FD93ED6 
------------------------------------------------------------------------------------------------------------------
Random Bitcoin Puzzle #68 Private Keys
Start key 0000000000000000000000000000000000000000000000080000000000000000 
Stop key 00000000000000000000000000000000000000000000000fffffffffffffffff 
80000000000000000...fffffffffffffffff (267...268)
P2PKH(c) 1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvvZ


Shāng (wèi):68 Zǒng jiàn shù:295147905179352825856 Sōusuǒ sùdù:2123 Měi miǎo M jiàn shù (10^6) tiān:1609073.819 Xīngqí:229867.688 Yuè:52791.136 Nián:4405.404
108 / 5,000
Entropy (bits): 68
Total keys: 295147905179352825856
Search speed: 2123 M keys per second (10^6)
Day: 1609073.819
Week: 229867.688
Month: 52791.136
Year: 4405.404

----------------------------------------------------------------------------------------------------------------
Check Bits: 28 \
Compressed Address: \
12jbtzBb54r97TCwW3G1gCFoumpckRAPdY \
Address hash160: \
1306b9e4ff56513a476841bac7ba48d69516b1da \
Secret wif: 2SaK6n3GY7WKrHRSyvhrn1k6zdzeAMmNBXH5PtXqoEYscExVUKsh2G \
Secret hex: \
0xd916ce8 \
pk: \
03e9e661838a96a65331637e2a3e948dc0756e5009e7cb5c36664d9b72dd18c0a7 

----------------------------------------------------------------------------------------------------------------

## Windows

Intall CUDA SDK and build OpenSSL, open VanitySearch.sln in Visual C++ 2017. \
You may need to reset your *Windows SDK version* in project properties. \
In Build->Configuration Manager, select the *Release* configuration. \
Build OpenSSL: \
Install Perl x64 for MS Windows from https://strawberryperl.com \
Link: https://strawberryperl.com/download/5.32.1.1/strawberry-perl-5.32.1.1-64bit.msi \
Install Netwide Assembler (NASM).  \
Download NASM x64 https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win64/nasm-2.16.01-installer-x64.exe \
Add Path C:\Program Files\NASM\; \
Add Path C:\Strawberry\perl\bin\; \
Add PATHEXT .PL; before .BAT; \
those. - .COM;.EXE;.PL;.BAT; \
And be sure to restart your PC. \
Download the library from the official website openssl-1.0.1a.tar.gz \
http://www.openssl.org/source/old/1.0.1/openssl-1.0.1a.tar.gz \
Unpack openssl-1.0.1a.tar.gz into the openssl-1.0.1a directory and copy its contents to the directory: \
c:\openssl-src-64 \
Run Command Prompt Visual Studio - x64 Native Tools Command Prompt for VS 2017 as administrator \
Run commands: \
cd C:\openssl-src-64 \
perl Configure VC-WIN64A --prefix=C:\Build-OpenSSL-VC-64 \
ms\do_win64a \
nmake -f ms\ntdll.mak \
nmake -f ms\ntdll.mak install \
nmake -f ms\ntdll.mak test 

Build OpenSSL complete. \
Connecting libraries to a project in Visual Studio 2017 Community. \
It's very simple! \
Go to Solution Explorer - Project Properties and select: 
1. Select C/C++ next: \
C/C++ - General - Additional directories for included files - Edit - Create a line and specify the path: \
C:\Build-OpenSSL-VC-64\include \
OK 
2. Select Linker next: \
Linker - Input - Additional dependencies - Edit and specify the path: \
c:\Build-OpenSSL-VC-64\lib\ssleay32.lib \
c:\Build-OpenSSL-VC-64\lib\libeay32.lib \
OK \
Project Properties - Apply - OK \
Build and enjoy.\
\
Note: The current relase has been compiled with CUDA SDK 10.2, if you have a different release of the CUDA SDK, you may need to update CUDA SDK paths in VanitySearch.vcxproj using a text editor. 
The current nvcc option are set up to architecture starting at 3.0 capability, for older hardware, add the desired compute capabilities to the list in GPUEngine.cu properties, CUDA C/C++, Device, Code Generation.

## Linux

Intall OpenSSL.\
Intall CUDA SDK.\
Depenging on the CUDA SDK version and on your Linux distribution you may need to install an older g++ (just for the CUDA SDK).\
Edit the makefile and set up the good CUDA SDK path and appropriate compiler for nvcc. 

```
CUDA       = /usr/local/cuda
CXXCUDA    = /usr/bin/g++
```

You can enter a list of architectrure (refer to nvcc documentation) if you have several GPU with different architecture. Compute capability 2.0 (Fermi) is deprecated for recent CUDA SDK.
VanitySearch need to be compiled and linked with a recent gcc (>=7). The current release has been compiled with gcc 7.3.0.\
Go to the VanitySearch directory. 
ccap is the desired compute capability https://ru.wikipedia.org/wiki/CUDA

```
$ g++ -v
gcc version 7.3.0 (Ubuntu 7.3.0-27ubuntu1~18.04)
$ make all (for build without CUDA support)
or
$   make gpu=1 ccap=86 all

```
Please fill in the form according to your graphics card computing capabilities.  ccap=86  3080 is 86, not 8.6. Remove the decimal point from the following values.

```
NVIDIA A100	8	RTX A5000	8.6	GeForce RTX 3090 Ti	8.6	GeForce RTX 3080 Ti	8.6
NVIDIA A40	8.6	RTX A4000	8.6	GeForce RTX 3090	8.6	GeForce RTX 3080	8.6
NVIDIA A30	8	RTX A3000	8.6	GeForce RTX 3080 Ti	8.6	GeForce RTX 3070 Ti	8.6
NVIDIA A10	8.6	RTX A2000	8.6	GeForce RTX 3080	8.6	GeForce RTX 3070	8.6
NVIDIA A16	8.6	RTX 5000	7.5	GeForce RTX 3070 Ti	8.6	GeForce RTX 3060	8.6
NVIDIA A2	8.6	RTX 4000	7.5	GeForce RTX 3070	8.6	GeForce RTX 3050 Ti	8.6
NVIDIA T4	7.5	RTX 3000	7.5	Geforce RTX 3060 Ti	8.6	GeForce RTX 3050	8.6
NVIDIA V100	7	T2000	7.5	Geforce RTX 3060	8.6	Geforce RTX 2080	7.5
Tesla P100	6	T1200	7.5	GeForce GTX 1650 Ti	7.5	Geforce RTX 2070	7.5
Tesla P40	6.1	T1000	7.5	NVIDIA TITAN RTX	7.5	Geforce RTX 2060	7.5
Tesla P4	6.1	T600	7.5	Geforce RTX 2080 Ti	7.5	GeForce GTX 1080	6.1
Tesla M60	5.2	T500	7.5	Geforce RTX 2080	7.5	GeForce GTX 1070	6.1
Tesla M40	5.2	P620	6.1	Geforce RTX 2070	7.5	GeForce GTX 1060	6.1
Tesla K80	3.7	P520	6.1	Geforce RTX 2060	7.5	GeForce GTX 980	5.2
Tesla K40	3.5	Quadro P5200	6.1	NVIDIA TITAN V	7	GeForce GTX 980M	5.2
Tesla K20	3.5	Quadro P4200	6.1	NVIDIA TITAN Xp	6.1	GeForce GTX 970M	5.2
Tesla K10	3	Quadro P3200	6.1	NVIDIA TITAN X	6.1	GeForce GTX 965M	5.2
RTX A6000	8.6	Quadro P5000	6.1	GeForce GTX 1080 Ti	6.1	GeForce GTX 960M	5
RTX A5000	8.6	Quadro P4000	6.1	GeForce GTX 1080	6.1	GeForce GTX 950M	5
RTX A4000	8.6	Quadro P3000	6.1	GeForce GTX 1070 Ti	6.1	GeForce 940M	5
T1000	7.5	Quadro P2000	6.1	GeForce GTX 1070	6.1	GeForce 930M	5
T600	7.5	Quadro P1000	6.1	GeForce GTX 1060	6.1	GeForce 920M	3.5
T400	7.5	Quadro P600	6.1	GeForce GTX 1050	6.1	GeForce 910M	5.2
Quadro RTX 8000	7.5	Quadro P500	6.1	GeForce GTX TITAN X	5.2	GeForce GTX 880M	3
Quadro RTX 6000	7.5	Quadro M5500M	5.2	GeForce GTX TITAN Z	3.5	GeForce GTX 870M	3
Quadro RTX 5000	7.5	Quadro M2200	5.2	GeForce GTX TITAN Black	3.5	GeForce GTX 860M	3.0/5.0
Quadro RTX 4000	7.5	Quadro M1200	5	GeForce GTX TITAN	3.5	GeForce GTX 850M	5
Quadro GV100	7	Quadro M620	5.2	GeForce GTX 980 Ti	5.2	GeForce 840M	5
Quadro GP100	6	Quadro M520	5	GeForce GTX 980	5.2	GeForce 830M	5
Quadro P6000	6.1	Quadro K6000M	3	GeForce GTX 970	5.2	GeForce 820M	2.1
Quadro P5000	6.1	Quadro K5200M	3	GeForce GTX 960	5.2	GeForce 800M	2.1
Quadro P4000	6.1	Quadro K5100M	3	GeForce GTX 950	5.2	GeForce GTX 780M	3
Quadro P2200	6.1	Quadro M5000M	5	GeForce GTX 780 Ti	3.5	GeForce GTX 770M	3
Quadro P2000	6.1	Quadro K500M	3	GeForce GTX 780	3.5	GeForce GTX 765M	3
Quadro P1000	6.1	Quadro K4200M	3	GeForce GTX 770	3	GeForce GTX 760M	3
Quadro P620	6.1	Quadro K4100M	3	GeForce GTX 760	3	GeForce GTX 680MX	3
Quadro P600	6.1	Quadro M4000M	5	GeForce GTX 750 Ti	5	GeForce GTX 680M	3
Quadro P400	6.1	Quadro K3100M	3	GeForce GTX 750	5	GeForce GTX 675MX	3
Quadro M6000 24GB	5.2	GeForce GT 730 DDR3,128bit	2.1	GeForce GTX 690	3	GeForce GTX 675M	2.1
Quadro M6000	5.2	Quadro M3000M	5	GeForce GTX 680	3	GeForce GTX 670MX	3
Quadro 410	3	Quadro K2200M	3	GeForce GTX 670	3	GeForce GTX 670M	2.1
Quadro K6000	3.5	Quadro K2100M	3	GeForce GTX 660 Ti	3	GeForce GTX 660M	3
Quadro M5000	5.2	Quadro M2000M	5	GeForce GTX 660	3	GeForce GT 755M	3
Quadro K5200	3.5	Quadro K1100M	3	GeForce GTX 650 Ti BOOST	3	GeForce GT 750M	3
Quadro K5000	3	Quadro M1000M	5	GeForce GTX 650 Ti	3	GeForce GT 650M	3
Quadro M4000	5.2	Quadro K620M	5	GeForce GTX 650	3	GeForce GT 745M	3
Quadro K4200	3	Quadro K610M	3.5	GeForce GTX 560 Ti	2.1	GeForce GT 645M	3
Quadro K4000	3	Quadro M600M	5	GeForce GTX 550 Ti	2.1	GeForce GT 740M	3
Quadro M2000	5.2	Quadro K510M	3.5	GeForce GTX 460	2.1	GeForce GT 730M	3
Quadro K2200	5	Quadro M500M	5	GeForce GTS 450	2.1	GeForce GT 640M	3
Quadro K2000	3	GeForce 705M	2.1	GeForce GTS 450*	2.1	GeForce GT 640M LE	3
Quadro K2000D	3	NVIDIA NVS 810	5	GeForce GTX 590	2	GeForce GT 735M	3
Quadro K1200	5	NVIDIA NVS 510	3	GeForce GTX 580	2	GeForce GT 635M	2.1
Quadro K620	5	NVIDIA NVS 315	2.1	GeForce GTX 570	2	GeForce GT 730M	3
Quadro K600	3	NVIDIA NVS 310	2.1	GeForce GTX 480	2	GeForce GT 630M	2.1
Quadro K420	3	Quadro Plex 7000	2	GeForce GTX 470	2	GeForce GT 625M	2.1
GeForce GT 730	3.5	GeForce 710M	2.1	GeForce GTX 465	2	GeForce GT 720M	2.1
GeForce GT 720	3.5	GeForce 610M	2.1	GeForce GT 740	3	GeForce GT 620M	2.1
```
# License

VanitySearch is licensed under GPLv3.

## Sponsorship

If this project has been helpful to you, please consider sponsoring. It is the greatest support for me, and I am deeply grateful. Thank you.

- **BTC**: bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
- **ETH**: 0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
- **DOGE**: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
- **TRX**: TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4

