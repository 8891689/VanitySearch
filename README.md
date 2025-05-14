# VanitySearch
Forked from https://github.com/JeanLucPons/VanitySearch 

Please note, this is not my project!
I only fixed the following issues:
1. Fixed the issue where the corresponding private key was incorrect when searching for vanity addresses.
2. Fixed the issue where the address was incorrect during puzzle search.
3. Fixed the issue where vanity address search was also correct, and addresses starting with 1, 3, or bc1 were correctly matched.
4. Fixed the error in converting the 16-bit private key to a WIF private key.
5. Fixed the issue of an insecure weak random number generator by replacing it with OpenSSL's random number generator.
6. Added a feature to divide the private key area for puzzle puzzles, adding -r to regenerate random numbers when the specified private key area reaches a certain quantity, improving the probability of puzzle hits. The more random, the slower (the smaller the value after -r, the slower),and this program or other programs can only use point addition ECC high-speed calculations, not normal standard calculations.

   
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
# CPU Vanity Address and Jigsaw Puzzle Instances
```
./VanitySearch -stop -t 4 -bits 28 -r 5000 -level 4 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 1461501637330902918203684832716283019655932542976
[🟑 ]Search: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY [Compressed]
[🟑 ]Start Thu May 15 00:33:42 2025
[🟑 ]Base Key: Randomly changed every 5000 Mkeys
[🟑 ]Number of CPU thread: 4
[🟑 ][Ts 3.44 Mkey/s][GPU 0.00 Mkey/s][Total 2^26.82][Prob 0.0%][50% in 9.33158e+33y][Found 0]
[🟐 ]Add: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
[🟐 ]WIF: KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M82GSgY8p5EkUe
[🟐 ]Key: 0xD916CE8


./VanitySearch -stop -t 4 -bits 28 -r 5000 -level 4 12jbtzBb54r97TCwW3G1gCFoumpc
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 164888818499126885406117494769938638116436836352
[🟑 ]Search: 12jbtzBb54r97TCwW3G1gCFoumpc [Compressed]
[🟑 ]Start Thu May 15 00:34:44 2025
[🟑 ]Base Key: Randomly changed every 5000 Mkeys
[🟑 ]Number of CPU thread: 4
[🟑 ][Ts 3.72 Mkey/s][GPU 0.00 Mkey/s][Total 2^24.41][Prob 0.0%][50% in 9.73392e+32y][Found 0]
[🟐 ]Add: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
[🟐 ]WIF: KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M82GSgY8p5EkUe
[🟐 ]Key: 0xD916CE8


./VanitySearch -stop -t 1 -bits 256 -r 500 -level 4 1btc
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 4553521
[🟑 ]Search: 1btc [Compressed]
[🟑 ]Start Thu May 15 00:35:20 2025
[🟑 ]Base Key: Randomly changed every 500 Mkeys
[🟑 ]Number of CPU thread: 1

[🟐 ]Add: 1btckBuYpVDUi1wwMAL6jdr4MYFCpATi7
[🟐 ]WIF: L2HPq7hA33mcyDTCYj9bTDwRrFRdjbBSNykx5RGfqp2rXKwYGzTn
[🟐 ]Key: 0x9714FA049957DB81F6269B68FA18F9F0B900F5EC5003D562D030760430559821
 
```
---------------------------------------------------------------------------------------------------------------------
# GPU Vanity Address and Jigsaw Puzzle Instances

```
./VanitySearch -stop -t 0 -gpu -bits 38 -r 50000 -level 4 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 1461501637330902918203684832716283019655932542976
[🟑 ]Search: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2 [Compressed]
[🟑 ]Start Thu May 15 00:35:49 2025
[🟑 ]Base Key: Randomly changed every 50000 Mkeys
[🟑 ]Number of CPU thread: 0
[🟑 ]GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
[🟑 ][Ts 2281.36 Mkey/s][GPU 2281.36 Mkey/s][Total 2^33.67][Prob 0.0%][50% in 1.40807e+31y][Found 0]
[🟐 ]Add: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
[🟐 ]WIF: KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9P3MahktLW5315v
[🟐 ]Key: 0x22382FACD0


/VanitySearch -stop -t 0 -gpu -bits 37 -r 100 -level 4 14iXhn8bGajVWegZHJ18vJLHhntcpL4dex
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 1461501637330902918203684832716283019655932542976
[🟑 ]Search: 14iXhn8bGajVWegZHJ18vJLHhntcpL4dex [Compressed]
[🟑 ]Start Thu May 15 00:36:19 2025
[🟑 ]Base Key: Randomly changed every 100 Mkeys
[🟑 ]Number of CPU thread: 0
[🟑 ]GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
[🟑 ][Ts 1354.55 Mkey/s][GPU 1354.55 Mkey/s][Total 2^33.92][Prob 0.0%][50% in 2.37151e+31y][Found 0]
[🟐 ]Add: 14iXhn8bGajVWegZHJ18vJLHhntcpL4dex
[🟐 ]WIF: KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9NRuiZFAX6XciCX
[🟐 ]Key: 0x1757756A93


./VanitySearch -t 0 -gpu -bits 256 -r 50000 -level 4 1Great
[🟑 ]OpenSSL level 4
[🟑 ]Difficulty: 264104224
[🟑 ]Search: 1Great [Compressed]
[🟑 ]Start Thu May 15 00:37:06 2025
[🟑 ]Base Key: Randomly changed every 50000 Mkeys
[🟑 ]Number of CPU thread: 0
[🟑 ]GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)

[🟐 ]Add: 1GreatkMjiCP3rA9RwCRZbCSmKSFaHzXur
[🟐 ]WIF: L3dBA7VRFycyKjBiJwSU21wUcDvP1v9Q3jJ2sN3LKSyx5PGUWoND
[🟐 ]Key: 0xBF187F66F8CE616AA55F3ED94703954CB579804E185BF7ECAC62C14C0DB7CB74

[🟐 ]Add: 1Greatkro8JmUxqVHnvxAehfof4kfqZxKk
[🟐 ]WIF: L4VTm41jEAycDA6EymcNqTzRHK9g7nC2i7vPKVGNCrUjkaRWZJKP
[🟐 ]Key: 0xD8F7000FFEE48AED5BCA77C656EF88503DC784F42D89E4F640B43FEC446D033A

[🟐 ]Add: 1GreatsZTqwdXHGVSUkPtuSuttpRzcxoDw
[🟐 ]WIF: L1euHYRFNjr8qWtE4WJsV1t14wn7cdsNGJCSTUg6Q2XFTbFtoKHZ
[🟐 ]Key: 0x844F1A8C9D564C327813664BBB88B5A7707B5E33BA30F9C6B58CB47F5CCDF6B8

[🟐 ]Add: 1Great1b32Lxue4bgtNaXMCTgVY1LZpCGK
[🟐 ]WIF: L2aXh7fzfiqcmbdBrqGKzMnoHpdqdGVaUNN78V6iBsdAHzGqX8ND
[🟐 ]Key: 0x9FE5A97ABA0C9B1EB68B3CC6C85812624145EA28AA67FE10895C8399F30469E1

[🟐 ]Add: 1GreatvNZ9pcocDJj1Y12AqsNh2S6VwKgQ
[🟐 ]WIF: L42V5LdLSSx2kkK52WMadNSKeEAnG8k13B1cVZPKQQu1dSEw4Exn
[🟐 ]Key: 0xCB163132E3E9751118A5449EA6059C73B12D88A1E6666FD0E084D1FB624C8D30

[🟐 ]Add: 1Greatj1CFUJKJ9FWvHJxLRGiJ4AmKxUN6
[🟐 ]WIF: L4HC6GXG7E2A1Ptv7b5pdecVvQ8M6FWLLJ7MgphfM6zdrWcY7DkZ
[🟐 ]Key: 0xD2A711C4E9EC11EF15D69AA5E06B527A3D5D873F0D725C9F68A933B799F1E4C9

[🟐 ]Add: 1GreatEP4RQvV184E7sgJQVBrc7nC8S6K1
[🟐 ]WIF: L1qxJqJ38BooFVY9LvPkYyyYHqZovqC4LuFyPSuzAz7L5jS7yiw2
[🟐 ]Key: 0x89FE9F879B268DC5A5FBC9902C408A223D4E3D356735D28D37A6498C8D6E185F

[🟐 ]Add: 1GreatT2TFamMdp5hoef8CK6sew3BQ2yt4
[🟐 ]WIF: L58vDJX7xNcwpNTznE8tP84EXhLFGqtKLw2qNmoGVjvPGAbrRvb8
[🟐 ]Key: 0xEC3BD1D1DA3D90EFE1962CB2D5435CB94ACB23948D140EC22F37722B4FFFB0B8

[🟐 ]Add: 1Greatpj4HZwv6VAXJJ6uWhMxbsppDqWpG
[🟐 ]WIF: L1dpt1wiRkLWtqToD97qEK5x2KV3a8bQePXXwRLPdhbsvryNKbQB
[🟐 ]Key: 0x83C167755C47939185869F558A5B9C9D176D3CE87B8AEBA847A9984D14C57C48

[🟐 ]Add: 1GreatNRcy7KcsYyyimabyjF5VrEP5SvH4
[🟐 ]WIF: L2u8bCcF5TAKPaU9iPzUGbs28YjuE3sYMaVequ7CNeVYSoqsu1Sj
[🟐 ]Key: 0xA9776E954ADA0EE0F0AE374934E495816162816E5AEFA0532808EAD60DE217D3

[🟐 ]Add: 1GreatqgZiMzuCmh3tSZioRTvK5F2RSJ6Y
[🟐 ]WIF: L4Brqt7ccX2cjAAoAS2wpdPG8M42az72s7gcnB8q6ZwBZMjDHdmP
[🟐 ]Key: 0xCFE8E2478996E9F306A5C439B2AFF53FDA3F328262978583E76CBBB2EC854345

[🟐 ]Add: 1GreatREtffVuThsdLqV4tm9ZsCj3Sf6H1
[🟐 ]WIF: L2eUU43EPK5owwCQY2nPaKavjDyax1P5mgzEqaULo8saaGrrXns2
[🟐 ]Key: 0xA1ED1F8745531C30161332EAB0607AD7A3390A5021F88D1489B541A739C2313A

[🟐 ]Add: 1GreatC4SomJgqaoxGKHQJpffWm2LPvJLD
[🟐 ]WIF: L4CbhEAfuXNnufjgLYJVn3xzTT3tMUivgBLahVAxW1b4z1iNPkxr
[🟐 ]Key: 0xD04A2E8E7AA89C85CE7D103B4EF76C343D598ADE3819C3605E4853685829F107
^C

The results are saved to Found.txt by default

------------------------------------------------------------------------------------------------------------------

Random Bitcoin Puzzle #68 Private Keys
Start key 0000000000000000000000000000000000000000000000080000000000000000 
Stop key 00000000000000000000000000000000000000000000000fffffffffffffffff 
80000000000000000...fffffffffffffffff (267...268)
P2PKH(c) 1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvvZ

```
```
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
```
## Windows
```
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
```
## Linux
```
Intall OpenSSL.\
Intall CUDA SDK.\
Depenging on the CUDA SDK version and on your Linux distribution you may need to install an older g++ (just for the CUDA SDK).\
Edit the makefile and set up the good CUDA SDK path and appropriate compiler for nvcc. 

CUDA       = /usr/local/cuda
CXXCUDA    = /usr/bin/g++


You can enter a list of architectrure (refer to nvcc documentation) if you have several GPU with different architecture. Compute capability 2.0 (Fermi) is deprecated for recent CUDA SDK.
VanitySearch need to be compiled and linked with a recent gcc (>=7). The current release has been compiled with gcc 7.3.0.\
Go to the VanitySearch directory. 
ccap is the desired compute capability https://ru.wikipedia.org/wiki/CUDA

Ubuntu/Debian system

make all (for build without CUDA support)

or

make gpu=0 ccap=86 all

Please fill in the form according to your graphics card computing capabilities.

ccap=86  3080 is 86, not 8.6. Remove the decimal point from the following values.


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
```
VanitySearch is licensed under GPLv3.
```
## Sponsorship

If this project has been helpful to you, please consider sponsoring. It is the greatest support for me, and I am deeply grateful. Thank you.
```
BTC:  bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
ETH:  0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
DOGE: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
TRX:  TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4
 ``` 
