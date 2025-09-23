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
7. Add -e option to enable internal homomorphism. This feature is disabled by default in some areas. You need to enable it actively to get high-speed operation. When adding an address, the program removes the value behind it (the amount value). Batch operations only support compressed and uncompressed types starting with 1, 3, and BC1Q. One of them, the address type! You need to extract addresses starting with 1 or 3.
8. Repaired and modified the core, optimized the doubling of the curve operation in terms of CPU support, and increased the speed several times. It is limited to executing tasks within the region to avoid internal homomorphism. The program is still in the experimental stage and there may be ignorance problems.
9. Added -a, -h, and -p options for batch or single searches, with auto-sorting and an AVX2 computation library, significantly boosting speed to become the fastest program on the market. For GPUs not at full load, add -g 512,256 and other grid types to fully load the cores. Only a suitable grid is the best choice. For GPU searches, use -r random mode; incremental mode is only for the CPU. Please use the compiled program, which is the latest version.

10. Verify the results

``` 
 ./VanitySearch -cp 1
Key  (HEX)  : KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU73sVHnoWn
Pub  (HEX)  : 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
Add (P2PKH) : 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
Add  (P2SH) : 3JvL6Ymt8MVWiCNHC7oWU6nLeHNJKLZGLN
Add (BECH32): bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4

```

```
./VanitySearch -help
VanitySearch v3.0 - Help

Usage:
  ./VanitySearch [options] [prefix]
  ./VanitySearch [options] -a <address_file>
  ./VanitySearch [options] -p <pubkey_file>
  ./VanitySearch [options] -h <hash160_file>

Core Options:
  [prefix]             Prefix to search (e.g., 1Bitcoin). Can contain wildcard '?' or '*'.
  -a <file>            Input file with list of address prefixes to search, one per line.
  -h <file>            Input file with HASH160s (hex), one per line.
  -p <file>            Input file with public key prefixes (hex), one per line.
  -o <file>            Output file for results (default: Results.txt).
  -stop                Stop when all targets from all input sources are found.

Search Mode:
  -u                   Search for uncompressed public keys/addresses (default: compressed).
  -b                   Search for both compressed and uncompressed keys/addresses.
  -c                   Case-insensitive search for address prefixes.

Performance & Hardware:
  -gpu                 Enable GPU acceleration.
  -t <threads>         Number of CPU threads to use (default: 0 if -gpu is used, 1 otherwise).
  -gpuId <id1,id2>     Comma-separated list of GPU device IDs to use (default: 0).
  -g <x1,y1,x2,y2>     Grid size (XxY) for each GPU (default: auto-configured).

Key Generation & Range:
  -s <seed>            Specify a seed for deterministic key generation.
  -r <Mkeys>           Enable random mode. Generates a new random key every <Mkeys> million keys.
                       Random keys are sampled within the specified range.
  -bits <N>            Search keys in the range [2^(N-1), 2^N-1].
  -area <hexA:hexB>    Search keys in the hexadecimal range from A to B.
  -e                   Enable performance optimizations (endomorphisms) when a specific key range
                       (-bits or -area) is set. By default, range search is strict.
  -sp <pubkey>         Start search from a base public key (for key splitting).

Utility Commands:
  -v                   Print version information.
  -l                   List available CUDA-enabled GPU devices.
  -check               Perform a self-check of CPU and GPU cryptographic functions.
  -kp [-s <seed>]      Generate a key pair from an optional seed.
  -cp <privkey>        Compute public key and addresses from a private key (WIF or hex).
  -ca <pubkey>         Compute addresses from a public key (hex).
  -rp <privkey> <file> Reconstruct final private key from a partial key file.

Technical Support      https://github.com/8891689

```

---------------------------------------------------------------------------------------------------------------------

# CPU Vanity Address and Jigsaw Puzzle Instances
```
The single-thread speed is slightly faster than keyhunt.

./VanitySearch -stop -t 1 -bits 28 -e -r 1 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY [Compressed]
❀  Start Mon May 19 14:45:34 2025
❀  Random mode
❀  Rekey every: 1 Mkeys
❀  Range
❀  from : 0x8000000
❀  to   : 0xFFFFFFF
❀  Number of CPU thread: 1
❀  [5.10 Mkey/s][GPU 0.00 Mkey/s][Total 2^26.18][Prob 0.0%][50% in 1.03724e+34y][Rekey 11][Found 0]  
✿  Add: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
✿  WIF: p2pkh:KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M82GSgY8p5EkUe
✿  KEY: 0xD916CE8


./VanitySearch -stop -t 4 -area 8000000:FFFFFFF -e -r 1 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY [Compressed]
❀  Start Mon May 19 14:49:25 2025
❀  Random mode
❀  Rekey every: 1 Mkeys
❀  Range
❀  from : 0x8000000
❀  to   : 0xFFFFFFF
❀  Number of CPU thread: 4
❀  [20.93 Mkey/s][GPU 0.00 Mkey/s][Total 2^25.73][Prob 0.0%][50% in 2.30679e+33y][Rekey 0][Found 0]  
✿  Add: 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY
✿  WIF: p2pkh:KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M82GSgY8p5EkUe
✿  KEY: 0xD916CE8


The keyhunt speed is 3M per second.

./keyhunt -m rmd160 -f 71.hash160.txt -l compress -t 1 -s 60 -R -r 400000000000000000:800000000000000000
[+] Version 0.2.230519 Satoshi Quest, developed by AlbertoBSD
[+] Mode rmd160
[+] Search compress only
[+] Thread : 1
[+] Stats output every 60 seconds
[+] Random mode
[+] N = 0x100000000
[+] Range 
[+] -- from : 0x400000000000000000
[+] -- to   : 0x800000000000000000
[+] Allocating memory for 1 elements: 0.00 MB
[+] Bloom filter for 1 elements.
[+] Loading data to the bloomfilter total: 0.03 MB
[+] Sorting data ... done! 1 values were loaded and sorted
^C] Total 1997834240 keys in 660 seconds: ~3 Mkeys/s (3027021 keys/s)


```

# GPU Vanity Address and Jigsaw Puzzle Instances

```
./VanitySearch -t 1 -gpu -gpuId 0 -bits 71 -e -r 999999 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU
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

./VanitySearch -t 1 -gpu -gpuId 0 -bits 71 -e -r 999999 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU
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

./VanitySearch -t 0 -gpu -gpuId 0 -area 400000000000000000:7fffffffffffffffff -e -r 999999 1PWo3JeB9jrGw
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


./VanitySearch -stop -t 4 -gpu -bits 38 -e -r 50000 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 1461501637330902918203684832716283019655932542976
❀  Search: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2 [Compressed]
❀  Start Mon May 19 14:52:39 2025
❀  Random mode
❀  Rekey every: 50000 Mkeys
❀  Range
❀  from : 0x2000000000
❀  to   : 0x3FFFFFFFFF
❀  Number of CPU thread: 4
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3407.76 Mkey/s][GPU 3395.37 Mkey/s][Total 2^38.21][Prob 0.0%][50% in 9.42646e+30y][Rekey 5][Found 0]  
✿  Add: 1HBtApAFA9B2YZw3G2YKSMCtb3dVnjuNe2
✿  WIF: p2pkh:KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9P3MahktLW5315v
✿  KEY: 0x22382FACD0



./VanitySearch -t 0 -e -gpu -gpuId 0 -r 8891689 18891689
❀  VanitySearch v2.0
❀  Check: No -o output file. Will save 'Results.txt'
❀  Difficulty: 888446610539
❀  Search: 18891689 [Compressed]
❀  Start Mon May 19 14:55:01 2025
❀  Random mode
❀  Rekey every: 8891689 Mkeys
❀  Range
❀  from : 0x1
❀  to   : 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140
❀  Number of CPU thread: 0
❀  GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x0 cores) Grid(544x128)
❀  [3502.32 Mkey/s][GPU 3502.32 Mkey/s][Total 2^37.89][Prob 24.9%][50% in 00:01:43][Rekey 0][Found 0]  
✿  Add: 18891689nzsjYYaPeY9b2Tmwzt44uCU6G8
✿  WIF: p2pkh:L5LPW7kZwuH84qf8dhPesgBGqFHPrkQ7vuVoJ6q8hbUEPuacCnXd
✿  KEY: 0xF22271147BDDBDA8ACEC59A840003127392A5F79BD825566507F314F74A9654E
❀  [3568.21 Mkey/s][GPU 3568.21 Mkey/s][Total 2^38.57][Prob 36.8%][50% in 00:00:59][Rekey 0][Found 1]  
✿  Add: 18891689Ws6YsaYuCRvCA6E9TSoFePcWvr
✿  WIF: p2pkh:L5SeKtprrPZdmVofL87F3M186UCtNcUGWusBWe9D695XUn3QUUAn
✿  KEY: 0xF55A4704A0389AB896C7E594BC721537A0F5100D308672DC3A5CD82DAA4FB997
❀  [3575.39 Mkey/s][GPU 3575.39 Mkey/s][Total 2^40.34][Prob 79.1%][80% in 00:00:11][Rekey 0][Found 2]  
✿  Add: 18891689zZBirLne4VRh9S8eh1ee6iSdG3
✿  WIF: p2pkh:L2XECP1h9TCwmLA82djoUxVmGRUNPEY7VXHmCyqBQfAsvXS2NKjg
✿  KEY: 0x9E32D92407AA4264956C246E99A8E4305A47C5DCD4B5DF94911051A305B8C612
❀  [3575.35 Mkey/s][GPU 3575.35 Mkey/s][Total 2^40.35][Prob 79.4%][80% in 00:00:07][Rekey 0][Found 3]  ^C


It is 1G/s faster than FixedPaul's VanitySearch-Bitcrack v2.10.

./vanitysearch -gpuId 0 -start 400000000000000000 -range 70 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU
VanitySearch-Bitcrack v2.10 by FixedPaul
[keyspace]  range=2^70
[keyspace]  start=400000000000000000
[keyspace]    end=7FFFFFFFFFFFFFFFFF
Search: 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU [Compressed]
Current task START time: Mon May 19 14:15:01 2025
GPU: GPU #0 NVIDIA GeForce RTX 3080 (68x128 cores) Grid(8192x256)
Starting keys set in 0.50 seconds  
2654.9 MK/s - 2^36.39 [0.00%] - RUN: 00:00:33.9|END: Too much bro - Found: 0  


```
The results are saved to Results.txt by default


# Fixed the problem of the original version. You can use the original version to search for Vanity addresses. After a slight optimization, the speed is still quite fast. And like the V2.0 version, it does not rely on other libraries.
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


----------------------------------------------------------------------------------------------------------------
# Compilation

## Windows
```
Intall CUDA SDK.
Install  g++ (just for the CUDA SDK).
```
## Linux
```
Intall CUDA SDK.
Install  g++ (just for the CUDA SDK). 
```


Ubuntu/Debian system

If you use Cuda12, you can use it directly. Please grant execution permission (if necessary):

```
chmod +x VanitySearch
```
Clean and recompile
```
make clean
```

If you don't know the architecture, just compile all and you can use GPU graphics cards of all architectures.
```
make gpu=0 all
```
or
```
make gpu=0 ccap=86 all
```
If you are compiling for a single GPU architecture, just copy the build file in the directory and add it to your build document. No changes are required for GPU compiling for all architectures.
Please fill in the form according to the compute capabilities of your graphics card.

ccap=86  3080 is 86, not 8.6. Remove the decimal point from the following values.

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
