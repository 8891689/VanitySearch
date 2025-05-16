# VanitySearch
Forked from https://github.com/JeanLucPons/VanitySearch 

Please note, this is not my project!
I only fixed the following issues:
1. Fixed the issue where the corresponding private key was incorrect when searching for vanity addresses.
2. Fixed the issue where the address was incorrect during puzzle search.
3. Fixed the issue where vanity address search was also correct, and addresses starting with 1, 3, or bc1 were correctly matched.
4. Fixed the error in converting the 16-bit private key to a WIF private key.
5. Fixed This issue was fixed by replacing the insecure weak random number generator.
6. Added a feature to divide the private key area for puzzle puzzles, adding -r to regenerate random numbers when the specified private key area reaches a certain quantity, improving the probability of puzzle hits. The more random, the slower (the smaller the value after -r, the slower),and this program or other programs can only use point addition ECC high-speed calculations, not normal standard calculations.


7. Verify the results

``` 
./VanitySearch -cp 1
PrivAddr: p2pkh:KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU73sVHnoWn
PubKey: 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
Addr (P2PKH): 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
Addr (P2SH): 3JvL6Ymt8MVWiCNHC7oWU6nLeHNJKLZGLN
Addr (BECH32): bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4
```

If you use the 86GPU architecture, you can use it directly. Please grant execution permissions (if necessary):
```
chmod +x VanitySearch
```
Clean and recompile
```
make clean

make gpu=0 ccap=86 all
```
```
./VanitySearch -h
VanitySeacrh [-check] [-v] [-u] [-b] [-c] [-gpu] [-stop] [-i inputfile]
             [-gpuId gpuId1[,gpuId2,...]] [-g g1x,g1y,[,g2x,g2y,...]]
             [-o outputfile] [-m maxFound] [-ps seed] [-s seed] [-t nbThread]
             [-nosse] [-r rekey] [-check] [-kp] [-sp startPubKey]
             [-rp privkey partialkeyfile]
             [-bits N] [-area A:B] [prefix]

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
 -r rekey: Rekey interval in MegaKey, default is disabled (deterministic search).
           When > 0, sample random keys within the specified range.
 -bits N: Search random keys in the range [2^(N-1), 2^N-1] (for rekey > 0 or restricted search).
 -area A:B: Search random keys in the hex range [A, B] (for rekey > 0 or restricted search).

```

--------------------------------------------------------------------------------------------------------------

 

---------------------------------------------------------------------------------------------------------------------
# GPU CPU Vanity Address and Jigsaw Puzzle Instances

```
./VanitySearch -t 1 -gpu -gpuId 0 -bits 71 -r 999999 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU [Compressed]
❀  Start Sat May 17 04:52:41 2025
❀  Random mode
❀  Rekey every: 999999 Mkeys
❀  Range
❀  from : 0x400000000000000000
❀  to   : 0x7FFFFFFFFFFFFFFFFF
❀  Number of CPU thread: 1
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [4283.79 Mkey/s][GPU 4277.65 Mkey/s][Total 2^33.00][Prob 0.0%][50% in 7.49876e+30y][Rekey 0][Found 0]  ^C

3080 after stabilization

./VanitySearch -t 1 -gpu -gpuId 0 -bits 71 -r 999999 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU [Compressed]
❀  Start Sat May 17 04:53:26 2025
❀  Random mode
❀  Rekey every: 999999 Mkeys
❀  Range
❀  from : 0x400000000000000000
❀  to   : 0x7FFFFFFFFFFFFFFFFF
❀  Number of CPU thread: 1
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3532.10 Mkey/s][GPU 3529.02 Mkey/s][Total 2^38.25][Prob 0.0%][50% in 9.09462e+30y][Rekey 0][Found 0]  ^C

./VanitySearch -t 0 -gpu -gpuId 0 -area 400000000000000000:7fffffffffffffffff -r 999999 1PWo3JeB9jrGw
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 10054102514374868992
❀  Search: 1Veryfierce [Compressed]
❀  Start Sat May 17 05:21:06 2025
❀  Random mode
❀  Rekey every: 999999 Mkeys
❀  Range
❀  from : 0x400000000000000000
❀  to   : 0x7FFFFFFFFFFFFFFFFF
❀  Number of CPU thread: 0
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3529.00 Mkey/s][GPU 3529.00 Mkey/s][Total 2^37.49][Prob 0.0%][50% in 62.6197y][Rekey 0][Found 0]  ^C


./VanitySearch -stop -t 4 -gpu -bits 38 -r 50000 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2 [Compressed]
❀  Start Sat May 17 04:44:52 2025
❀  Random mode
❀  Rekey every: 50000 Mkeys
❀  Range
❀  from : 0x2000000000
❀  to   : 0x3FFFFFFFFF
❀  Number of CPU thread: 4
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3407.76 Mkey/s][GPU 3395.42 Mkey/s][Total 2^40.17][Prob 0.0%][50% in 9.42647e+30y][Rekey 23][Found 0]  
✿  Add: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
✿  WIF: p2pkh:KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9P3MahktLW5315v
✿  KEY: 0x22382FACD0


./VanitySearch -t 0 -gpu -gpuId 0 -r 8891689 18891689
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 888446610539
❀  Search: 18891689 [Compressed]
❀  Start Sat May 17 05:27:56 2025
❀  Random mode
❀  Rekey every: 8891689 Mkeys
❀  Range
❀  from : 0x1
❀  to   : 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140
❀  Number of CPU thread: 0
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3502.31 Mkey/s][GPU 3502.31 Mkey/s][Total 2^37.85][Prob 24.3%][50% in 00:01:45][Rekey 0][Found 0]  
✿  Add: 18891689DmVW4NwCWrs9pDg8ML9WNH8iSy
✿  WIF: p2pkh:L2km6Bqw7FG3KZQr87atDNNjUEw4QrqUARj9ym6YPpZCERyqwD1r
✿  KEY: 0xA5290B57B705CE014C9992754893FA86C76DDF646D4BA7B0B52FAF3E42F975F0
❀  [3525.58 Mkey/s][GPU 3525.58 Mkey/s][Total 2^40.05][Prob 72.3%][80% in 00:01:23][Rekey 0][Found 1]  
✿  Add: 18891689PQsZH8Y4CUqBfHCMSBr8eTxx6F
✿  WIF: p2pkh:L4PvBFTeedgr5nv445qDThuooykJQ5Njo79dnRg4pd499i7TkVNM
✿  KEY: 0xD61CCEA13142CD65A8106A4805E66B5A2BB21D7146E111DB8A8BF6505F16241B
❀  [3525.57 Mkey/s][GPU 3525.57 Mkey/s][Total 2^40.62][Prob 85.1%][90% in 00:01:41][Rekey 0][Found 2]  
✿  Add: 18891689uUPn46wojBoJPR29KTyQ5FKJ3e
✿  WIF: p2pkh:KyS3ocNPL9qXWfWUV2oQ6KtUmdgKMQLT9z7A5wkXGMMoHgvgBe8w
✿  KEY: 0x4205A646C594E9460D5028F7EC210E81FE1614BCDCBB9252D4F01BDDAD02507C
❀  [3575.56 Mkey/s][GPU 3575.56 Mkey/s][Total 2^40.65][Prob 85.6%][90% in 00:01:33][Rekey 0][Found 3]  ^C


Please note that single CPU calculation will result in an error. You can only use GPU and CPU, or run with GPU alone. We will fix single CPU operation when we have time.For those who do not want to run the jigsaw puzzle game, or do not have a limited area search, please use my V1.0 version, which contains the repaired original version.

```
The results are saved to Results.txt by default


# The original version has been repaired, and you can use the original version to search for Vanity addresses. It has been slightly optimized and the speed is still quite fast, and you don’t have to rely on the library.
```
./VanitySearch -stop -t 0 -gpu bc1qmqzlduj
VanitySearch v1.19
Difficulty: 34359738368
Search: bc1qmqzlduj [Compressed]
Start Thu May 15 03:31:04 2025
Base Key: 38A731F4740F67F685BB2AB453EC8167CDF2922A96075C900309E2750B4BB36D
Number of CPU thread: 0
GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
[3635.93 Mkey/s][GPU 3635.93 Mkey/s][Total 2^35.57][Prob 77.3%][80% in 00:00:01][Found 0]  
PubAddress: bc1qmqzldujhxc6pk677nk8v58hhec07fnl98695pu
Priv (WIF): p2wpkh:L3uDSAXcFB2YnPQ2upSgAyeurAks7TWaR5rebduQp5ZbxuC1aLWB
Priv (HEX): 0xC758CE0B8BF098097A44D54BAC137E96EC3C4ABBA1D943ABBCC87C17C4E8A286

./VanitySearch -cp C758CE0B8BF098097A44D54BAC137E96EC3C4ABBA1D943ABBCC87C17C4E8A286
PrivAddr: p2pkh:L3uDSAXcFB2YnPQ2upSgAyeurAks7TWaR5rebduQp5ZbxuC1aLWB
PubKey: 0369B34E4D57F230A63D1FCD389F0ED85FC9A94C167DA5D4F523BAF3D6663835FC
Addr (P2PKH): 1LhE6sCY97uMAiF28MiT4BikaUPJin5Zip
Addr (P2SH): 3N9FcVopVd2oP5QxQRwGYPNHyEWZmpfQzh
Addr (BECH32): bc1qmqzldujhxc6pk677nk8v58hhec07fnl98695pu

```



# Dependencies

The v2.0 version does not require any dependencies and can be compiled directly.

Debian/Ubuntu based systems (such as Debian if you are using it):
```
sudo apt update
sudo apt install libssl-dev
```

Fedora/CentOS/RHEL based systems:
```
sudo yum install openssl-devel
```

or for newer Fedora/CentOS Stream/RHEL 8+
```
sudo dnf install openssl-devel
```

Arch Linux based systems:
```
sudo pacman -S openssl
```

macOS (using Homebrew):
```
brew install openssl
```
```
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
