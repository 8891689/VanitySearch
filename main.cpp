/*
 * This file is part of the VanitySearch distribution (https://github.com/JeanLucPons/VanitySearch).
 * Copyright (c) 2019 Jean Luc PONS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 
 * Copyright (c) 2025 8891689
 * https://github.com/8891689
 * This code file contains modifications of the original work (Copyright (c) 2019 Jean Luc PONS).
*/
#include "Timer.h"
#include "Vanity.h"
#include "SECP256k1.h"
#include <fstream>
#include <string>
#include <string.h>
#include <stdexcept>
#include "hash/sha512.h"
#include "hash/sha256.h"
#include "bitrange.h" 

#define RELEASE "2.0"

using namespace std;

// --- Helper functions for parsing range (implementing bitrange.h functionality here) ---
// Note: In a real project, these might be in bitrange.cpp

// Sets range to [2^(bits-1), 2^bits - 1]
int parseBitRange(int bits, Int* min_val, Int* max_val) {
    if (bits <= 0) {
        fprintf(stderr, "Error: -bits argument must be positive.\n");
        return -1;
    }
    if (bits > Int::NB64BLOCK * 64) { // Check against max possible bits Int can handle
        fprintf(stderr, "Error: -bits argument (%d) is too large for Int type.\n", bits);
        return -1;
    }

    // min = 2^(bits - 1)
    min_val->SetInt32(1);
    if (bits > 1) {
      min_val->ShiftL(bits - 1);
    }
    // max = 2^bits - 1
    max_val->SetInt32(1);
    max_val->ShiftL(bits);
    max_val->SubOne();

    return 0; // Success
}

// Sets range from hex string A:B
int parseHexRange(const string& range_str, Int* min_val, Int* max_val) {
    size_t colon_pos = range_str.find(':');
    if (colon_pos == string::npos || colon_pos == 0 || colon_pos == range_str.length() - 1) {
        fprintf(stderr, "Error: -area argument must be in format A:B (hex values).\n");
        return -1;
    }

    string min_hex = range_str.substr(0, colon_pos);
    string max_hex = range_str.substr(colon_pos + 1);

    try {
        min_val->SetBase16((char*)min_hex.c_str()); // SetBase16 expects char*
        max_val->SetBase16((char*)max_hex.c_str()); // SetBase16 expects char*
    } catch (...) { // Basic catch for potential parsing errors
         fprintf(stderr, "Error: Invalid hex value in -area argument.\n");
         return -1;
    }


    return 0; // Success
}
// --- End Helper functions ---


// ------------------------------------------------------------------------------------------

void printUsage() {

  printf("VanitySeacrh [-check] [-v] [-u] [-b] [-c] [-gpu] [-stop] [-i inputfile]\n");
  printf("             [-gpuId gpuId1[,gpuId2,...]] [-g g1x,g1y,[,g2x,g2y,...]]\n");
  printf("             [-o outputfile] [-m maxFound] [-ps seed] [-s seed] [-t nbThread]\n");
  printf("             [-nosse] [-r rekey] [-check] [-kp] [-sp startPubKey]\n");
  printf("             [-rp privkey partialkeyfile]\n");
  printf("             [-bits N] [-area A:B] [prefix]\n\n"); // <-- 添加新参数说明
  printf(" prefix: prefix to search (Can contains wildcard '?' or '*')\n");
  printf(" -v: Print version\n");
  printf(" -u: Search uncompressed addresses\n");
  printf(" -b: Search both uncompressed or compressed addresses\n");
  printf(" -c: Case unsensitive search\n");
  printf(" -gpu: Enable gpu calculation\n");
  printf(" -stop: Stop when all prefixes are found\n");
  printf(" -i inputfile: Get list of prefixes to search from specified file\n");
  printf(" -o outputfile: Output results to the specified file\n");
  printf(" -gpu gpuId1,gpuId2,...: List of GPU(s) to use, default is 0\n");
  printf(" -g g1x,g1y,g2x,g2y, ...: Specify GPU(s) kernel gridsize, default is 8*(MP number),128\n");
  printf(" -m: Specify maximun number of prefixes found by each kernel call\n");
  printf(" -s seed: Specify a seed for the base key, default is random\n");
  printf(" -ps seed: Specify a seed concatened with a crypto secure random seed\n");
  printf(" -t threadNumber: Specify number of CPU thread, default is number of core\n");
  printf(" -nosse: Disable SSE hash function\n");
  printf(" -l: List cuda enabled devices\n");
  printf(" -check: Check CPU and GPU kernel vs CPU\n");
  printf(" -cp privKey: Compute public key (privKey in hex hormat)\n");
  printf(" -ca pubKey: Compute address (pubKey in hex hormat)\n");
  printf(" -kp: Generate key pair\n");
  printf(" -rp privkey partialkeyfile: Reconstruct final private key(s) from partial key(s) info.\n");
  printf(" -sp startPubKey: Start the search with a pubKey (for private key splitting)\n");
  printf(" -r rekey: Rekey interval in MegaKey, default is disabled (deterministic search).\n");
  printf("           When > 0, sample random keys within the specified range.\n");
  printf(" -bits N: Search random keys in the range [2^(N-1), 2^N-1] (for rekey > 0 or restricted search).\n"); // <-- 新参数说明
  printf(" -area A:B: Search random keys in the hex range [A, B] (for rekey > 0 or restricted search).\n"); // <-- 新参数说明
  exit(0);

}
// ------------------------------------------------------------------------------------------

int getInt(string name,char *v) {

  int r;

  try {

    r = std::stoi(string(v));

  } catch(std::invalid_argument&) {

    printf("Invalid %s argument, number expected\n",name.c_str());
    exit(-1);

  }

  return r;

}

// ------------------------------------------------------------------------------------------

void getInts(string name,vector<int> &tokens, const string &text, char sep) {

  size_t start = 0, end = 0;
  tokens.clear();
  int item;

  try {

    while ((end = text.find(sep, start)) != string::npos) {
      item = std::stoi(text.substr(start, end - start));
      tokens.push_back(item);
      start = end + 1;
    }

    item = std::stoi(text.substr(start));
    tokens.push_back(item);

  } catch(std::invalid_argument &) {

    printf("Invalid %s argument, number expected\n",name.c_str());
    exit(-1);

  }

}

// ------------------------------------------------------------------------------------------

void parseFile(string fileName, vector<string> &lines) {

  // Get file size
  FILE *fp = fopen(fileName.c_str(), "rb");
  if (fp == NULL) {
    printf("Error: Cannot open %s %s\n", fileName.c_str(), strerror(errno));
    exit(-1);
  }
  fseek(fp, 0L, SEEK_END);
  size_t sz = ftell(fp);
  size_t nbAddr = sz / 33; /* Upper approximation */
  bool loaddingProgress = sz > 100000;
  fclose(fp);

  // Parse file
  int nbLine = 0;
  string line;
  ifstream inFile(fileName);
  lines.reserve(nbAddr);
  while (getline(inFile, line)) {

    // Remove ending \r\n
    int l = (int)line.length() - 1;
    while (l >= 0 && isspace(line.at(l))) {
      line.pop_back();
      l--;
    }

    if (line.length() > 0) {
      lines.push_back(line);
      nbLine++;
      if (loaddingProgress) {
        if ((nbLine % 50000) == 0)
          printf("[Loading input file %5.1f%%]\r", ((double)nbLine*100.0) / ((double)(nbAddr)*33.0 / 34.0));
      }
    }

  }

  if (loaddingProgress)
    printf("[Loading input file 100.0%%]\n");

}

// ------------------------------------------------------------------------------------------

void generateKeyPair(Secp256K1 *secp, string seed, int searchMode,bool paranoiacSeed) {

  if (seed.length() < 8) {
    printf("Error: Use a seed of at leats 8 characters to generate a key pair\n");
    printf("Ex: VanitySearch -s \"A Strong Password\" -kp\n");
    exit(-1);
  }

  if(paranoiacSeed)
    seed = seed + Timer::getSeed(32);

  if (searchMode == SEARCH_BOTH) {
    printf("Error: Use compressed or uncompressed to generate a key pair\n");
    exit(-1);
  }

  bool compressed = (searchMode == SEARCH_COMPRESSED);

  string salt = "VanitySearch";
  unsigned char hseed[64];
  pbkdf2_hmac_sha512(hseed, 64, (const uint8_t *)seed.c_str(), seed.length(),
    (const uint8_t *)salt.c_str(), salt.length(),
    2048);

  Int privKey;
  privKey.SetInt32(0);
  sha256(hseed, 64, (unsigned char *)privKey.bits64);
  Point p = secp->ComputePublicKey(&privKey);
  printf("Priv : %s\n", secp->GetPrivAddress(compressed,privKey).c_str());
  printf("Pub  : %s\n", secp->GetPublicKeyHex(compressed,p).c_str());

}

// ------------------------------------------------------------------------------------------

void outputAdd(string outputFile, int addrType, string addr, string pAddr, string pAddrHex) {

  FILE *f = stdout;
  bool needToClose = false;

  if (outputFile.length() > 0) {
    f = fopen(outputFile.c_str(), "a");
    if (f == NULL) {
      printf("Cannot open %s for writing\n", outputFile.c_str());
      f = stdout;
    } else {
      needToClose = true;
    }
  }

  fprintf(f, "\n✿  Add: %s\n", addr.c_str());


  switch (addrType) {
  case P2PKH:
    fprintf(f, "✿  WIF: p2pkh:%s\n", pAddr.c_str());
    break;
  case P2SH:
    fprintf(f, "✿  WIF: p2wpkh-p2sh:%s\n", pAddr.c_str());
    break;
  case BECH32:
    fprintf(f, "✿  WIF: p2wpkh:%s\n", pAddr.c_str());
    break;
  }
  fprintf(f, "✿  KEY: 0x%s\n", pAddrHex.c_str());

  if (needToClose)
    fclose(f);

}

// ------------------------------------------------------------------------------------------
#define CHECK_ADDR()                                           \
  fullPriv.ModAddK1order(&e, &partialPrivKey);                 \
  p = secp->ComputePublicKey(&fullPriv);                       \
  cAddr = secp->GetAddress(addrType, compressed, p);           \
  if (cAddr == addr) {                                         \
    found = true;                                              \
    string pAddr = secp->GetPrivAddress(compressed, fullPriv); \
    string pAddrHex = fullPriv.GetBase16();                    \
    outputAdd(outputFile, addrType, addr, pAddr, pAddrHex);    \
  }

void reconstructAdd(Secp256K1 *secp, string fileName, string outputFile, string privAddr) {

  bool compressed;
  int addrType;
  Int lambda;
  Int lambda2;
  lambda.SetBase16("5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72");
  lambda2.SetBase16("ac9c52b33fa3cf1f5ad9e3fd77ed9ba4a880b9fc8ec739c2e0cfc810b51283ce");

  Int privKey = secp->DecodePrivateKey((char *)privAddr.c_str(),&compressed);
  if(privKey.IsNegative())
    exit(-1);

  vector<string> lines;
  parseFile(fileName,lines);

  for (int i = 0; i < (int)lines.size(); i+=2) {

    string addr;
    string partialPrivAddr;

    if (lines[i].substr(0, 12) == "PubAddress: ") {

      addr = lines[i].substr(12);

      switch (addr.data()[0]) {
      case '1':
        addrType = P2PKH; break;
      case '3':
        addrType = P2SH; break;
      case 'b':
      case 'B':
        addrType = BECH32; break;
      default:
        printf("Invalid partialkey info file at line %d\n", i);
        printf("%s Address format not supported\n", addr.c_str());
        continue;
      }

    } else {
      printf("Invalid partialkey info file at line %d (\"PubAddress: \" expected)\n",i);
      exit(-1);
    }

    if (lines[i+1].substr(0, 13) == "PartialPriv: ") {
      partialPrivAddr = lines[i+1].substr(13);
    } else {
      printf("Invalid partialkey info file at line %d (\"PartialPriv: \" expected)\n", i);
      exit(-1);
    }

    bool partialMode;
    Int partialPrivKey = secp->DecodePrivateKey((char *)partialPrivAddr.c_str(), &partialMode);
    if (privKey.IsNegative()) {
      printf("Invalid partialkey info file at line %d\n", i);
      exit(-1);
    }

    if (partialMode != compressed) {

      printf("Warning, Invalid partialkey at line %d (Wrong compression mode, ignoring key)\n", i);
      continue;

    } else {

      // Reconstruct the address
      Int fullPriv;
      Point p;
      Int e;
      string cAddr;
      bool found = false;

      // No sym, no endo
      e.Set(&privKey);
      CHECK_ADDR();

      // No sym, endo 1
      e.Set(&privKey);
      e.ModMulK1order(&lambda);
      CHECK_ADDR();

      // No sym, endo 2
      e.Set(&privKey);
      e.ModMulK1order(&lambda2);
      CHECK_ADDR();

      // sym, no endo
      e.Set(&privKey);
      e.Neg();
      e.Add(&secp->order);
      CHECK_ADDR();

      // sym, endo 1
      e.Set(&privKey);
      e.ModMulK1order(&lambda);
      e.Neg();
      e.Add(&secp->order);
      CHECK_ADDR();

      // sym, endo 2
      e.Set(&privKey);
      e.ModMulK1order(&lambda2);
      e.Neg();
      e.Add(&secp->order);
      CHECK_ADDR();

      if (!found) {
        printf("Unable to reconstruct final key from partialkey line %d\n Addr: %s\n PartKey: %s\n",
          i, addr.c_str(),partialPrivAddr.c_str());
      }

    }

  }

}

// ------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {

  // Global Init
  Timer::Init();


  // Init SecpK1
  Secp256K1 *secp = new Secp256K1();
  secp->Init();

  //GPUEngine::GenerateCode(secp,512);
  //exit(0);

  // --- 初始化默认范围 [1, N-1] ---
  Int min_range;
  Int max_range;
  Int one_int((uint64_t)1); // 用于设置 min_range 的默认值 1

  // 获取曲线阶数 N
  Int N_order = secp->order;
  Int N_minus_one = N_order;
  N_minus_one.SubOne(); // 计算 N-1

  // 设置默认范围 [1, N-1]
  min_range.Set(&one_int);
  max_range.Set(&N_minus_one);
  // --- 结束新增 ---

  // Browse arguments
  if (argc < 2) {
    printf("Error: No arguments (use -h for help)\n");
    exit(-1);
  }

  int a = 1;
  bool gpuEnable = false;
  bool stop = false;
  int searchMode = SEARCH_COMPRESSED;
  vector<int> gpuId = {0};
  vector<int> gridSize;
  string seed = "";
  vector<string> prefix;
  string outputFile = "";
  int nbCPUThread = Timer::getCoreNumber();
  bool tSpecified = false;
  bool sse = true;
  uint32_t maxFound = 65536;
  uint64_t rekey = 0;
  Point startPuKey;
  startPuKey.Clear();
  bool startPubKeyCompressed;
  bool caseSensitive = true;
  bool paranoiacSeed = false;
  bool range_specified = false; // 通过 -bits 或 -area 指定范围

  while (a < argc) { 

    if (strcmp(argv[a], "-gpu")==0) {
      gpuEnable = true;
      a++;
    } else if (strcmp(argv[a], "-gpuId")==0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -gpuId requires an argument.\n"); exit(-1); }
      getInts("gpuId",gpuId,string(argv[a]),',');
      a++;
    } else if (strcmp(argv[a], "-stop") == 0) {
      stop = true;
      a++;
    } else if (strcmp(argv[a], "-c") == 0) {
      caseSensitive = false;
      a++;
    } else if (strcmp(argv[a], "-v") == 0) {
      printf("%s\n",RELEASE);
      exit(0);
    } else if (strcmp(argv[a], "-check") == 0) {

      Int::Check();
      secp->Check();

#ifdef WITHGPU
      if (gridSize.size() == 0) {
        gridSize.push_back(-1);
        gridSize.push_back(128);
      }
      GPUEngine g(gridSize[0],gridSize[1],gpuId[0],maxFound,false);
      g.SetSearchMode(searchMode);
      g.Check(secp);
#else
  printf("GPU code not compiled, use -DWITHGPU when compiling.\n");
#endif
      exit(0);
    } else if (strcmp(argv[a], "-l") == 0) {

#ifdef WITHGPU
      GPUEngine::PrintCudaInfo();
#else
  printf("GPU code not compiled, use -DWITHGPU when compiling.\n");
#endif
      exit(0);

    } else if (strcmp(argv[a], "-kp") == 0) {
      generateKeyPair(secp,seed,searchMode,paranoiacSeed);
      exit(0);
    } else if (strcmp(argv[a], "-sp") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -sp requires an argument.\n"); exit(-1); }
      string pub = string(argv[a]);
      startPuKey = secp->ParsePublicKeyHex(pub, startPubKeyCompressed);
      a++;
    } else if(strcmp(argv[a],"-ca") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -ca requires an argument.\n"); exit(-1); }
      string pub = string(argv[a]);
      bool isComp;
      Point p = secp->ParsePublicKeyHex(pub,isComp);
      printf("Addr (P2PKH): %s\n",secp->GetAddress(P2PKH,isComp,p).c_str());
      printf("Addr (P2SH): %s\n",secp->GetAddress(P2SH,isComp,p).c_str());
      printf("Addr (BECH32): %s\n",secp->GetAddress(BECH32,isComp,p).c_str());
      exit(0);
    } else if (strcmp(argv[a], "-cp") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -cp requires an argument.\n"); exit(-1); }
      string priv = string(argv[a]);
      Int k;
      bool isComp = true;
      if(priv[0]=='5' || priv[0] == 'K' || priv[0] == 'L') {
        k = secp->DecodePrivateKey((char *)priv.c_str(),&isComp);
      } else {
        k.SetBase16(argv[a]);
      }
      Point p = secp->ComputePublicKey(&k);
      printf("PrivAddr: p2pkh:%s\n",secp->GetPrivAddress(isComp,k).c_str());
      printf("PubKey: %s\n",secp->GetPublicKeyHex(isComp,p).c_str());
      printf("Addr (P2PKH): %s\n", secp->GetAddress(P2PKH,isComp,p).c_str());
      printf("Addr (P2SH): %s\n", secp->GetAddress(P2SH,isComp,p).c_str());
      printf("Addr (BECH32): %s\n", secp->GetAddress(BECH32,isComp,p).c_str());
      exit(0);
    } else if (strcmp(argv[a], "-rp") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -rp requires two arguments.\n"); exit(-1); }
      string priv = string(argv[a]);
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -rp requires two arguments.\n"); exit(-1); }
      string file = string(argv[a]);
      a++;
      reconstructAdd(secp,file,outputFile,priv);
      exit(0);
    } else if (strcmp(argv[a], "-u") == 0) {
      searchMode = SEARCH_UNCOMPRESSED;
      a++;
    } else if (strcmp(argv[a], "-b") == 0) {
      searchMode = SEARCH_BOTH;
      a++;
    } else if (strcmp(argv[a], "-nosse") == 0) {
      sse = false;
      a++;
    } else if (strcmp(argv[a], "-g") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -g requires an argument.\n"); exit(-1); }
      getInts("gridSize",gridSize,string(argv[a]),',');
      a++;
    } else if (strcmp(argv[a], "-s") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -s requires an argument.\n"); exit(-1); }
      seed = string(argv[a]);
      a++;
    } else if (strcmp(argv[a], "-ps") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -ps requires an argument.\n"); exit(-1); }
      seed = string(argv[a]);
      paranoiacSeed = true;
      a++;
    } else if (strcmp(argv[a], "-o") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -o requires an argument.\n"); exit(-1); }
      outputFile = string(argv[a]);
      a++;
    } else if (strcmp(argv[a], "-i") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -i requires an argument.\n"); exit(-1); }
      parseFile(string(argv[a]),prefix);
      a++;
    } else if (strcmp(argv[a], "-t") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -t requires an argument.\n"); exit(-1); }
      nbCPUThread = getInt("nbCPUThread",argv[a]);
      a++;
      tSpecified = true;
    } else if (strcmp(argv[a], "-m") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -m requires an argument.\n"); exit(-1); }
      maxFound = getInt("maxFound", argv[a]);
      a++;
    } else if (strcmp(argv[a], "-r") == 0) {
      a++;
      if (a >= argc) { fprintf(stderr, "Error: -r requires an argument.\n"); exit(-1); }
      rekey = (uint64_t)getInt("rekey", argv[a]);
      a++;
    } else if (strcmp(argv[a], "-h") == 0) {
      printUsage(); // This calls exit(0)
    } else if (strcmp(argv[a], "-bits") == 0) { 
      a++; // Move to the value
      if (a >= argc) { fprintf(stderr, "Error: -bits requires an argument (N).\n"); exit(-1); }
      if (range_specified) { fprintf(stderr, "Error: -bits and -area are mutually exclusive.\n"); exit(-1); }
      int bits = getInt("bits", argv[a]);
      if (parseBitRange(bits, &min_range, &max_range) != 0) {
          exit(-1); // parseBitRange already printed error
      }
      range_specified = true;
      a++; // Move to the next argument
    } else if (strcmp(argv[a], "-area") == 0) { 
      a++; // Move to the value
      if (a >= argc) { fprintf(stderr, "Error: -area requires an argument (A:B).\n"); exit(-1); }
       if (range_specified) { fprintf(stderr, "Error: -bits and -area are mutually exclusive.\n"); exit(-1); }
      if (parseHexRange(string(argv[a]), &min_range, &max_range) != 0) {
           exit(-1); // parseHexRange already printed error
      }
      range_specified = true;
      a++; // Move to the next argument
    }
    
    else { // Handle any other unexpected arguments or the final prefix
       if (a == argc - 1 && prefix.empty() && argv[a][0] != '-') {
           // This looks like the final prefix argument
           prefix.push_back(string(argv[a]));
           a++;
       } else {
           // Handle any other unexpected arguments
           printf("Unexpected %s argument\n",argv[a]);
           exit(-1);
       }
    }
    // --- 修正结束 ---

  } // <--- **ADD THIS CLOSING BRACE** - while 循环结束


  // 范围验证 ---
  // 确保最终确定的范围是有效的: 1 <= min <= max < N
  // 1. min_range >= 1
  // 2. min_range <= max_range
  // 3. max_range < N_order
  Int one_int_again((uint64_t)1); // 确保有一个 1 的 Int 对象
  if (min_range.IsZero() || min_range.IsLower(&one_int_again) || min_range.IsGreater(&max_range) || max_range.IsGreaterOrEqual(&N_order)) {
       fprintf(stderr, "Fatal Error: Invalid private key range specified or derived [%s, %s]. Must be 1 <= min <= max < N (%s).\n",
               min_range.GetBase16().c_str(), max_range.GetBase16().c_str(), N_order.GetBase16().c_str());
       exit(-1);
  }
  // --- 结束范围验证 ---


  printf("❀  VanitySearch v" RELEASE "\n");

  if(gridSize.size()==0) {
    for (int i = 0; i < gpuId.size(); i++) {
      gridSize.push_back(-1);
      gridSize.push_back(128);
    }
  } else if(gridSize.size() != gpuId.size()*2) {
    printf("Invalid gridSize or gpuId argument, must have coherent size\n");
    exit(-1);
  }

  // Let one CPU core free per gpu is gpu is enabled
  // It will avoid to hang the system
  if( !tSpecified && nbCPUThread>1 && gpuEnable)
    nbCPUThread-=(int)gpuId.size();
  if(nbCPUThread<0)
    nbCPUThread = 0;

  // If a starting public key is specified, force the search mode according to the key
  if (!startPuKey.isZero()) {
    searchMode = (startPubKeyCompressed)?SEARCH_COMPRESSED:SEARCH_UNCOMPRESSED;
  }

  // 创建 VanitySearch 对象，传递范围参数
  VanitySearch *v = new VanitySearch(secp, prefix, seed, searchMode, gpuEnable, stop,
                                     outputFile, sse, maxFound, rekey, caseSensitive,
                                     startPuKey, paranoiacSeed, min_range, max_range);

  v->Search(nbCPUThread,gpuId,gridSize);

  return 0;
}
