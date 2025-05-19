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
#include "Vanity.h"
#include "Base58.h"
#include "Bech32.h"
#include "hash/sha256.h"
#include "hash/sha512.h"
#include "IntGroup.h"
#include "Wildcard.h"
#include "Timer.h"
#include "hash/ripemd160.h"
#include <string.h>
#include <math.h>
#include <algorithm>
#ifndef WIN64
#include <pthread.h>
#endif

#include <iostream> // for std::cout
#include <iomanip>  // for std::setw, std::setfill
#include <sstream> // for std::stringstream

using namespace std;

Point Gn[CPU_GRP_SIZE / 2];
Point _2Gn;

const std::string DEFAULT_OUTPUT_FILENAME = "Results.txt";
// ----------------------------------------------------------------------------

// 构造函数实现
VanitySearch::VanitySearch(Secp256K1 *secp, vector<std::string> &inputPrefixes, std::string seed, int searchMode,
                           bool useGpu, bool stop, std::string userOutputFile, bool useSSE, uint32_t maxFound, // 將參數名改為 userOutputFile 以示區分
                           uint64_t rekey, bool caseSensitive, Point &startPubKey, bool paranoiacSeed,
                           const Int& min_r, const Int& max_r)
  :inputPrefixes(inputPrefixes), min_range(min_r), max_range(max_r)
{
  this->secp = secp;
  this->searchMode = searchMode;
  this->useGpu = useGpu;
  this->stopWhenFound = stop;
  this->rekeyCount = 0;

  // 決定最終的輸出檔案名
  if (userOutputFile.empty()) {
    this->outputFile = DEFAULT_OUTPUT_FILENAME; // 使用預設檔名
    printf("❀  Check: No -o output file. Will save '%s'\n", this->outputFile.c_str());
  } else {
    this->outputFile = userOutputFile; // 使用使用者指定的檔名
  }

  this->useSSE = useSSE;
  this->nbGPUThread = 0;
  this->maxFound = maxFound;
  this->rekey = rekey;
  this->searchType = -1;
  this->startPubKey = startPubKey;
  this->hasPattern = false;
  this->caseSensitive = caseSensitive;
  this->startPubKeySpecified = !startPubKey.isZero();

  lastRekey = 0;
  prefixes.clear();

  // Create a 65536 items lookup table
  PREFIX_TABLE_ITEM t;
  t.found = true;
  t.items = NULL;
  for(int i=0;i<65536;i++)
    prefixes.push_back(t);

  // Check is inputPrefixes contains wildcard character
  for (int i = 0; i < (int)inputPrefixes.size() && !hasPattern; i++) {
    hasPattern = ((inputPrefixes[i].find('*') != std::string::npos) ||
                   (inputPrefixes[i].find('?') != std::string::npos) );
  }

  if (!hasPattern) {

    // No wildcard used, standard search
    // Insert prefixes
    bool loadingProgress = (inputPrefixes.size() > 1000);
    if (loadingProgress)
      printf("[Building lookup16   0.0%%]\r");

    nbPrefix = 0;
    onlyFull = true;
    for (int i = 0; i < (int)inputPrefixes.size(); i++) {

      PREFIX_ITEM it;
      std::vector<PREFIX_ITEM> itPrefixes;

      if (!caseSensitive) {

        // For caseunsensitive search, loop through all possible combination
        // and fill up lookup table
        vector<string> subList;
        enumCaseUnsentivePrefix(inputPrefixes[i], subList);

        bool *found = new bool;
        *found = false;

        for (int j = 0; j < (int)subList.size(); j++) {
          if (initPrefix(subList[j], &it)) {
            it.found = found;
            it.prefix = strdup(it.prefix); // We need to allocate here, subList will be destroyed
            itPrefixes.push_back(it);
          }
        }

        if (itPrefixes.size() > 0) {

          // Compute difficulty for case unsensitive search
          // Not obvious to perform the right calculation here using standard double
          // Improvement are welcome

          // Get the min difficulty and divide by the number of item having the same difficulty
          // Should give good result when difficulty is large enough
          double dMin = itPrefixes[0].difficulty;
          int nbMin = 1;
          for (int j = 1; j < (int)itPrefixes.size(); j++) {
            if (itPrefixes[j].difficulty == dMin) {
              nbMin++;
            } else if (itPrefixes[j].difficulty < dMin) {
              dMin = itPrefixes[j].difficulty;
              nbMin = 1;
            }
          }

          dMin /= (double)nbMin;

          // Updates
          for (int j = 0; j < (int)itPrefixes.size(); j++)
            itPrefixes[j].difficulty = dMin;

        }

      } else {

        if (initPrefix(inputPrefixes[i], &it)) {
          bool *found = new bool;
          *found = false;
          it.found = found;
          itPrefixes.push_back(it);
        }

      }

      if (itPrefixes.size() > 0) {

        // Add the item to all correspoding prefixes in the lookup table
        for (int j = 0; j < (int)itPrefixes.size(); j++) {

          prefix_t p = itPrefixes[j].sPrefix;

          if (prefixes[p].items == NULL) {
            prefixes[p].items = new vector<PREFIX_ITEM>();
            prefixes[p].found = false;
            usedPrefix.push_back(p);
          }
          (*prefixes[p].items).push_back(itPrefixes[j]);

        }

        onlyFull &= it.isFull;
        nbPrefix++;

      }

      if (loadingProgress && i % 1000 == 0)
        printf("[Building lookup16 %5.1f%%]\r", (((double)i) / (double)(inputPrefixes.size() - 1)) * 100.0);
    }

    if (loadingProgress)
      printf("\n");

    //dumpPrefixes();

    if (!caseSensitive && searchType == BECH32) {
      printf("Error, case unsensitive search with BECH32 not allowed.\n");
      exit(1);
    }

    if (nbPrefix == 0) {
      printf("VanitySearch: nothing to search !\n");
      exit(1);
    }

    // Second level lookup
    uint32_t unique_sPrefix = 0;
    uint32_t minI = 0xFFFFFFFF;
    uint32_t maxI = 0;
    for (int i = 0; i < (int)prefixes.size(); i++) {
      if (prefixes[i].items) {
        LPREFIX lit;
        lit.sPrefix = i;
        if (prefixes[i].items) {
          for (int j = 0; j < (int)prefixes[i].items->size(); j++) {
            lit.lPrefixes.push_back((*prefixes[i].items)[j].lPrefix);
          }
        }
        sort(lit.lPrefixes.begin(), lit.lPrefixes.end());
        usedPrefixL.push_back(lit);
        if ((uint32_t)lit.lPrefixes.size() > maxI) maxI = (uint32_t)lit.lPrefixes.size();
        if ((uint32_t)lit.lPrefixes.size() < minI) minI = (uint32_t)lit.lPrefixes.size();
        unique_sPrefix++;
      }
      if (loadingProgress)
        printf("[Building lookup32 %.1f%%]\r", ((double)i*100.0) / (double)prefixes.size());
    }

    if (loadingProgress)
      printf("\n");

    _difficulty = getDiffuclty();
    string seachInfo = string(searchModes[searchMode]) + (startPubKeySpecified ? ", with public key" : "");
    if (nbPrefix == 1) {
      if (!caseSensitive) {
        // Case unsensitive search
        printf("❀  Difficulty: %.0f\n", _difficulty);
        printf("❀  Search: %s [%s, Case unsensitive] (Lookup size %d)\n", inputPrefixes[0].c_str(), seachInfo.c_str(), unique_sPrefix);
      } else {
        printf("❀  Difficulty: %.0f\n", _difficulty);
        printf("❀  Search: %s [%s]\n", inputPrefixes[0].c_str(), seachInfo.c_str());
      }
    } else {
      if (onlyFull) {
        printf("❀  Search: %d addresses (Lookup size %d,[%d,%d]) [%s]\n", nbPrefix, unique_sPrefix, minI, maxI, seachInfo.c_str());
      } else {
        printf("❀  Search: %d prefixes (Lookup size %d) [%s]\n", nbPrefix, unique_sPrefix, seachInfo.c_str());
      }
    }

  } else {

    // Wild card search
    switch (inputPrefixes[0].data()[0]) {

    case '1':
      searchType = P2PKH;
      break;
    case '3':
      searchType = P2SH;
      break;
    case 'b':
    case 'B':
      searchType = BECH32;
      break;

    default:
      printf("Invalid start character 1,3 or b, expected");
      exit(1);

    }

    string searchInfo = string(searchModes[searchMode]) + (startPubKeySpecified ? ", with public key" : "");
    if (inputPrefixes.size() == 1) {
      printf("❀  Search: %s [%s]\n", inputPrefixes[0].c_str(), searchInfo.c_str());
    } else {
      printf("❀  Search: %d patterns [%s]\n", (int)inputPrefixes.size(), searchInfo.c_str());
    }

    patternFound = (bool *)malloc(inputPrefixes.size()*sizeof(bool));
    memset(patternFound,0, inputPrefixes.size() * sizeof(bool));

  }

  // Compute Generator table G[n] = (n+1)*G

  Point g = secp->G;
  Gn[0] = g;
  g = secp->DoubleDirect(g);
  Gn[1] = g;
  for (int i = 2; i < CPU_GRP_SIZE/2; i++) {
    g = secp->AddDirect(g,secp->G);
    Gn[i] = g;
  }
  // _2Gn = CPU_GRP_SIZE*G
  _2Gn = secp->DoubleDirect(Gn[CPU_GRP_SIZE/2-1]);

  // Constant for endomorphism
  // if a is a nth primitive root of unity, a^-1 is also a nth primitive root.
  // beta^3 = 1 mod p implies also beta^2 = beta^-1 mop (by multiplying both side by beta^-1)
  // (beta^3 = 1 mod p),  beta2 = beta^-1 = beta^2
  // (lambda^3 = 1 mod n), lamba2 = lamba^-1 = lamba^2
  beta.SetBase16("7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee");
  lambda.SetBase16("5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72");
  beta2.SetBase16("851695d49a83f8ef919bb86153cbcb16630fb68aed0a766a3ec693d68e6afa40");
  lambda2.SetBase16("ac9c52b33fa3cf1f5ad9e3fd77ed9ba4a880b9fc8ec739c2e0cfc810b51283ce");

  // Seed
  if (seed.length() == 0) {
    // Default seed
    seed = Timer::getSeed(32);
  }

  if (paranoiacSeed) {
    seed += Timer::getSeed(32);
  }

  // Protect seed against "seed search attack" using pbkdf2_hmac_sha512
  string salt = "VanitySearch";
  unsigned char hseed[64];
  pbkdf2_hmac_sha512(hseed, 64, (const uint8_t *)seed.c_str(), seed.length(),
    (const uint8_t *)salt.c_str(), salt.length(),
    2048);
  startKey.SetInt32(0);
  sha256(hseed, 64, (unsigned char *)startKey.bits64);
// --- 檢查初始 startKey 是否在指定範圍內 (當 rekey == 0 時) ---
// 如果指定了範圍，並且不是 rekey 模式，檢查通過種子生成的起始密钥是否在範圍內
if (rekey == 0) { // 如果是 rekey > 0，這裡的 startKey 只是第一次 rekey 的隨機起點，後面會重新生成
   // 驗證 startKey 是否在 [min_range, max_range] 且非零
   // startKey 必須小於曲線階數 N
   Int one((uint64_t)1);
   if (startKey.IsZero() || startKey.IsLower(&min_range) || startKey.IsGreater(&max_range) || startKey.IsGreaterOrEqual(&secp->order) ) {
       fprintf(stderr, "Error: Initial key derived from seed (0x%s) is outside the specified range [0x%s, 0x%s] or invalid.\n", // 打印為 0x 前綴更清晰
               startKey.GetBase16().c_str(), min_range.GetBase16().c_str(), max_range.GetBase16().c_str());
       fprintf(stderr, "For searching a restricted range deterministically (rekey=0), the seed must yield a key within that range.\n");
       fprintf(stderr, "Consider using rekey > 0 to sample randomly from the range.\n");
       exit(-1); // 致命錯誤，退出
   }
}
// --- 結束檢查 ---

char *ctimeBuff;
time_t now = time(NULL);
ctimeBuff = ctime(&now);
printf("❀  Start %s", ctimeBuff); // ctime() 返回的字符串末尾已經有 '\n'

// 1. 啟用隨機模式 / 確定性模式
if (rekey > 0) {
    printf("❀  Random mode\n");
    // 2. 多少私鑰匙隨機一次 (僅在隨機模式下打印)
    printf("❀  Rekey every: %.0f Mkeys\n", (double)rekey);
} else {
    printf("❀  Deterministic mode (from seed)\n");
    // 在確定性模式下，可以打印由種子生成的起始 startKey
    printf("❀  Initial key: 0x%s\n", startKey.GetBase16().c_str());
}
// 打印範圍信息
printf("❀  Range\n");
// 3. 範圍開始
printf("❀  from : 0x%s\n", min_range.GetBase16().c_str());
// 4. 範圍結束
printf("❀  to   : 0x%s\n", max_range.GetBase16().c_str());

} // 函數結束

// ----------------------------------------------------------------------------

bool VanitySearch::isSingularPrefix(std::string pref) {

  // check is the given prefix contains only 1
  bool only1 = true;
  int i=0;
  while (only1 && i < (int)pref.length()) {
    only1 = pref.data()[i] == '1';
    i++;
  }
  return only1;

}

// ----------------------------------------------------------------------------
bool VanitySearch::initPrefix(std::string &prefix,PREFIX_ITEM *it) {

  std::vector<unsigned char> result;
  string dummy1 = prefix;
  int nbDigit = 0;
  bool wrong = false;

  if (prefix.length() < 2) {
    printf("Ignoring prefix \"%s\" (too short)\n",prefix.c_str());
    return false;
  }

  int aType = -1;


  switch (prefix.data()[0]) {
  case '1':
    aType = P2PKH;
    break;
  case '3':
    aType = P2SH;
    break;
  case 'b':
  case 'B':
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
    if(strncmp(prefix.c_str(), "bc1q", 4) == 0)
      aType = BECH32;
    break;
  }

  if (aType==-1) {
    printf("Ignoring prefix \"%s\" (must start with 1 or 3 or bc1q)\n", prefix.c_str());
    return false;
  }

  if (searchType == -1) searchType = aType;
  if (aType != searchType) {
    printf("Ignoring prefix \"%s\" (P2PKH, P2SH or BECH32 allowed at once)\n", prefix.c_str());
    return false;
  }

  if (aType == BECH32) {

    // BECH32
    uint8_t witprog[40];
    size_t witprog_len;
    int witver;
    const char* hrp = "bc";

    int ret = segwit_addr_decode(&witver, witprog, &witprog_len, hrp, prefix.c_str());

    // Try to attack a full address ?
    if (ret && witprog_len==20) {

      // mamma mia !
      it->difficulty = pow(2, 160);
      it->isFull = true;
      memcpy(it->hash160, witprog, 20);
      it->sPrefix = *(prefix_t *)(it->hash160);
      it->lPrefix = *(prefixl_t *)(it->hash160);
      it->prefix = (char *)prefix.c_str();
      it->prefixLength = (int)prefix.length();
      return true;

    }

    if (prefix.length() < 5) {
      printf("Ignoring prefix \"%s\" (too short, length<5 )\n", prefix.c_str());
      return false;
    }

    if (prefix.length() >= 36) {
      printf("Ignoring prefix \"%s\" (too long, length>36 )\n", prefix.c_str());
      return false;
    }

    uint8_t data[64];
    memset(data,0,64);
    size_t data_length;
    if(!bech32_decode_nocheck(data,&data_length,prefix.c_str()+4)) {
      printf("Ignoring prefix \"%s\" (Only \"023456789acdefghjklmnpqrstuvwxyz\" allowed)\n", prefix.c_str());
      return false;
    }

    // Difficulty
    it->sPrefix = *(prefix_t *)data;
    it->difficulty = pow(2, 5*(prefix.length()-4));
    it->isFull = false;
    it->lPrefix = 0;
    it->prefix = (char *)prefix.c_str();
    it->prefixLength = (int)prefix.length();

    return true;

  } else {

    // P2PKH/P2SH

    wrong = !DecodeBase58(prefix, result);

    if (wrong) {
      if (caseSensitive)
        printf("Ignoring prefix \"%s\" (0, I, O and l not allowed)\n", prefix.c_str());
      return false;
    }

    // Try to attack a full address ?
    if (result.size() > 21) {

      // mamma mia !
      //if (!secp.CheckPudAddress(prefix)) {
      //  printf("Warning, \"%s\" (address checksum may never match)\n", prefix.c_str());
      //}
      it->difficulty = pow(2, 160);
      it->isFull = true;
      memcpy(it->hash160, result.data() + 1, 20);
      it->sPrefix = *(prefix_t *)(it->hash160);
      it->lPrefix = *(prefixl_t *)(it->hash160);
      it->prefix = (char *)prefix.c_str();
      it->prefixLength = (int)prefix.length();
      return true;

    }

    // Prefix containing only '1'
    if (isSingularPrefix(prefix)) {

      if (prefix.length() > 21) {
        printf("Ignoring prefix \"%s\" (Too much 1)\n", prefix.c_str());
        return false;
      }

      // Difficulty
      it->difficulty = pow(256, prefix.length() - 1);
      it->isFull = false;
      it->sPrefix = 0;
      it->lPrefix = 0;
      it->prefix = (char *)prefix.c_str();
      it->prefixLength = (int)prefix.length();
      return true;

    }

    // Search for highest hash160 16bit prefix (most probable)

    while (result.size() < 25) {
      DecodeBase58(dummy1, result);
      if (result.size() < 25) {
        dummy1.append("1");
        nbDigit++;
      }
    }

    if (searchType == P2SH) {
      if (result.data()[0] != 5) {
        if(caseSensitive)
          printf("Ignoring prefix \"%s\" (Unreachable, 31h1 to 3R2c only)\n", prefix.c_str());
        return false;
      }
    }

    if (result.size() != 25) {
      printf("Ignoring prefix \"%s\" (Invalid size)\n", prefix.c_str());
      return false;
    }

    //printf("VanitySearch: Found prefix %s\n",GetHex(result).c_str() );
    it->sPrefix = *(prefix_t *)(result.data() + 1);

    dummy1.append("1");
    DecodeBase58(dummy1, result);

    if (result.size() == 25) {
      //printf("VanitySearch: Found prefix %s\n", GetHex(result).c_str());
      it->sPrefix = *(prefix_t *)(result.data() + 1);
      nbDigit++;
    }

    // Difficulty
    it->difficulty = pow(2, 192) / pow(58, nbDigit);
    it->isFull = false;
    it->lPrefix = 0;
    it->prefix = (char *)prefix.c_str();
    it->prefixLength = (int)prefix.length();

    return true;

  }
}

// ----------------------------------------------------------------------------

void VanitySearch::dumpPrefixes() {

  for (int i = 0; i < 0xFFFF; i++) {
    if (prefixes[i].items) {
      printf("%04X\n", i);
      for (int j = 0; j < (int)prefixes[i].items->size(); j++) {
        printf("  %d\n", (*prefixes[i].items)[j].sPrefix);
        printf("  %g\n", (*prefixes[i].items)[j].difficulty);
        printf("  %s\n", (*prefixes[i].items)[j].prefix);
      }
    }
  }

}
// ----------------------------------------------------------------------------

void VanitySearch::enumCaseUnsentivePrefix(std::string s, std::vector<std::string> &list) {

  char letter[64];
  int letterpos[64];
  int nbLetter = 0;
  int length = (int)s.length();

  for (int i = 1; i < length; i++) {
    char c = s.data()[i];
    if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ) {
      letter[nbLetter] = tolower(c);
      letterpos[nbLetter] = i;
      nbLetter++;
    }
  }

  int total = 1 << nbLetter;

  for (int i = 0; i < total; i++) {

    char tmp[64];
    strcpy(tmp, s.c_str());

    for (int j = 0; j < nbLetter; j++) {
      int mask = 1 << j;
      if (mask&i) tmp[letterpos[j]] = toupper(letter[j]);
      else         tmp[letterpos[j]] = letter[j];
    }

    list.push_back(string(tmp));

  }

}

// ----------------------------------------------------------------------------

double VanitySearch::getDiffuclty() {

  double min = pow(2,160);

  if (onlyFull)
    return min;

  for (int i = 0; i < (int)usedPrefix.size(); i++) {
    int p = usedPrefix[i];
    if (prefixes[p].items) {
      for (int j = 0; j < (int)prefixes[p].items->size(); j++) {
        if (!*((*prefixes[p].items)[j].found)) {
          if ((*prefixes[p].items)[j].difficulty < min)
            min = (*prefixes[p].items)[j].difficulty;
        }
      }
    }
  }

  return min;

}

double log1(double x) {
  // Use taylor series to approximate log(1-x)
  return -x - (x*x)/2.0 - (x*x*x)/3.0 - (x*x*x*x)/4.0;
}

string VanitySearch::GetExpectedTime(double keyRate,double keyCount) {

  char tmp[128];
  string ret;

  if(hasPattern)
    return "";

  double P = 1.0/ _difficulty;
  // pow(1-P,keyCount) is the probality of failure after keyCount tries
  double cP = 1.0 - pow(1-P,keyCount);

  sprintf(tmp,"[Prob %.1f%%]",cP*100.0);
  ret = string(tmp);

  double desiredP = 0.5;
  while(desiredP<cP)
    desiredP += 0.1;
  if(desiredP>=0.99) desiredP = 0.99;
  double k = log(1.0-desiredP)/log(1.0-P);
  if (isinf(k)) {
    // Try taylor
    k = log(1.0 - desiredP)/log1(P);
  }
  double dTime = (k-keyCount)/keyRate; // Time to perform k tries

  if(dTime<0) dTime = 0;

  double nbDay  = dTime / 86400.0;
  if (nbDay >= 1) {

    double nbYear = nbDay/365.0;
    if (nbYear > 1) {
      if(nbYear<5)
        sprintf(tmp, "[%.f%% in %.1fy]", desiredP*100.0, nbYear);
      else
        sprintf(tmp, "[%.f%% in %gy]", desiredP*100.0, nbYear);
    } else {
      sprintf(tmp, "[%.f%% in %.1fd]", desiredP*100.0, nbDay);
    }

  } else {

    int iTime = (int)dTime;
    int nbHour = (int)((iTime % 86400) / 3600);
    int nbMin = (int)(((iTime % 86400) % 3600) / 60);
    int nbSec = (int)(iTime % 60);

    sprintf(tmp, "[%.f%% in %02d:%02d:%02d]", desiredP*100.0, nbHour, nbMin, nbSec);

  }

  return ret + string(tmp);

}

// ----------------------------------------------------------------------------

void VanitySearch::output(std::string addr, std::string pAddr, std::string pAddrHex) {

#ifdef WIN64
   WaitForSingleObject(ghMutex, INFINITE);
#else
  pthread_mutex_lock(&ghMutex);
#endif

  // --- 準備要輸出的內容 ---
  // 為了避免重複格式化，我們先把內容格式化到一個字符串緩衝區
  // 或者，我們可以多次調用 fprintf/printf，但要注意內容一致性
  // 這裡選擇多次調用，因為內容格式相對固定
  
  // --- 始終打印到控制台 ---
  printf("\n"); // 加一個換行，讓控制台輸出更清晰
  printf("✿  Add: %s\n", addr.c_str());
  if (startPubKeySpecified) {
    printf("✿  KEY: %s\n", pAddr.c_str());
  } else {
    switch (searchType) {
    case P2PKH:
      printf("✿  WIF: p2pkh:%s\n", pAddr.c_str());
      break;
    case P2SH:
      printf("✿  WIF: p2wpkh-p2sh:%s\n", pAddr.c_str());
      break;
    case BECH32:
      printf("✿  WIF: p2wpkh:%s\n", pAddr.c_str());
      break;
    default:
      printf("✿  WIF: unknown_type:%s\n", pAddr.c_str());
      break;
    }
    printf("✿  KEY: 0x%s\n", pAddrHex.c_str());
  }
  fflush(stdout); // 確保控制台輸出立即顯示

  // --- 如果指定了文件，則同時打印到文件 ---
  if (!this->outputFile.empty()) { // this->outputFile 是由構造函數設置的最終文件名
    FILE *f = fopen(this->outputFile.c_str(), "a"); // 追加模式
    if (f != NULL) {
      // 成功打開文件，寫入內容
      fprintf(f, "✿  Add: %s\n", addr.c_str());
      if (startPubKeySpecified) {
        fprintf(f, "✿  KEY: %s\n", pAddr.c_str());
      } else {
        switch (searchType) {
        case P2PKH:
          fprintf(f, "✿  WIF: p2pkh:%s\n", pAddr.c_str());
          break;
        case P2SH:
          fprintf(f, "✿  WIF: p2wpkh-p2sh:%s\n", pAddr.c_str());
          break;
        case BECH32:
          fprintf(f, "✿  WIF: p2wpkh:%s\n", pAddr.c_str());
          break;
        default:
          fprintf(f, "✿  WIF: unknown_type:%s\n", pAddr.c_str());
          break;
        }
        fprintf(f, "✿  KEY: 0x%s\n", pAddrHex.c_str());
      }
      fprintf(f, "\n"); // 在文件中每條記錄後加一個額外換行，方便閱讀
      fflush(f);     // 確保內容寫入文件
      fclose(f);     // 關閉文件
    } else {
      // 文件打開失敗，在控制台打印錯誤信息（之前可能已經打印過，但這裡可以再提示一次）
      // 已經在控制台打印過主要內容了，所以這裡只打印文件錯誤
      printf("Error: Could not write to output file '%s'. Error: %s\n",
             this->outputFile.c_str(), strerror(errno));
    }
  }

#ifdef WIN64
  ReleaseMutex(ghMutex);
#else
  pthread_mutex_unlock(&ghMutex);
#endif
}

// ----------------------------------------------------------------------------

void VanitySearch::updateFound() {

  // Check if all prefixes has been found
  // Needed only if stopWhenFound is asked
  if (stopWhenFound) {

    if (hasPattern) {

      bool allFound = true;
      for (int i = 0; i < (int)inputPrefixes.size(); i++) {
        allFound &= patternFound[i];
      }
      endOfSearch = allFound;

    } else {

      bool allFound = true;
      for (int i = 0; i < (int)usedPrefix.size(); i++) {
        bool iFound = true;
        prefix_t p = usedPrefix[i];
        if (!prefixes[p].found) {
          if (prefixes[p].items) {
            for (int j = 0; j < (int)prefixes[p].items->size(); j++) {
              iFound &= *((*prefixes[p].items)[j].found);
            }
          }
          prefixes[usedPrefix[i]].found = iFound;
        }
        allFound &= iFound;
      }
      endOfSearch = allFound;

      // Update difficulty to the next most probable item
      _difficulty = getDiffuclty();

    }

  }

}

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
bool VanitySearch::checkPrivKey(string addr, Int &key, int32_t incr, int endomorphism, bool mode) {

  Int k(&key);
  Point sp = startPubKey;

  if (incr < 0) {
    k.Add((uint64_t)(-incr));
    k.Neg();
    k.Add(&secp->order);
    if (startPubKeySpecified) sp.y.ModNeg();
  } else {
    k.Add((uint64_t)incr);
  }

  // Endomorphisms
  switch (endomorphism) {
  case 1:
    k.ModMulK1order(&lambda);
    if(startPubKeySpecified) sp.x.ModMulK1(&beta);
    break;
  case 2:
    k.ModMulK1order(&lambda2);
    if (startPubKeySpecified) sp.x.ModMulK1(&beta2);
    break;
  }

  // Check addresses
  Point p = secp->ComputePublicKey(&k);
  if (startPubKeySpecified) p = secp->AddDirect(p, sp);

  string chkAddr = secp->GetAddress(searchType, mode, p);
  if (chkAddr != addr) {

    //Key may be the opposite one (negative zero or compressed key)
    k.Neg();
    k.Add(&secp->order);
    p = secp->ComputePublicKey(&k);
    if (startPubKeySpecified) {
      sp.y.ModNeg();
      p = secp->AddDirect(p, sp);
    }
    string chkAddr = secp->GetAddress(searchType, mode, p);
    if (chkAddr != addr) {
      printf("\nWarning, wrong private key generated !\n");
      printf("  Addr :%s\n", addr.c_str());
      printf("  Check:%s\n", chkAddr.c_str());
      printf("  Endo:%d incr:%d comp:%d\n", endomorphism, incr, mode);
      return false;
    }

  }

  output(addr, secp->GetPrivAddress(mode ,k), k.GetBase16());

  return true;

}

void VanitySearch::checkAddrSSE(uint8_t *h1, uint8_t *h2, uint8_t *h3, uint8_t *h4,
                                int32_t incr1, int32_t incr2, int32_t incr3, int32_t incr4,
                                Int &key, int endomorphism, bool mode) {

  vector<string> addr = secp->GetAddress(searchType, mode, h1,h2,h3,h4);

  for (int i = 0; i < (int)inputPrefixes.size(); i++) {

    if (Wildcard::match(addr[0].c_str(), inputPrefixes[i].c_str(), caseSensitive)) {

      // Found it !
      //*((*pi)[i].found) = true;
      if (checkPrivKey(addr[0], key, incr1, endomorphism, mode)) {
        nbFoundKey++;
        patternFound[i] = true;
        updateFound();
      }

    }

    if (Wildcard::match(addr[1].c_str(), inputPrefixes[i].c_str(), caseSensitive)) {

      // Found it !
      //*((*pi)[i].found) = true;
      if (checkPrivKey(addr[1], key, incr2, endomorphism, mode)) {
        nbFoundKey++;
        patternFound[i] = true;
        updateFound();
      }

    }

    if (Wildcard::match(addr[2].c_str(), inputPrefixes[i].c_str(), caseSensitive)) {

      // Found it !
      //*((*pi)[i].found) = true;
      if (checkPrivKey(addr[2], key, incr3, endomorphism, mode)) {
        nbFoundKey++;
        patternFound[i] = true;
        updateFound();
      }

    }

    if (Wildcard::match(addr[3].c_str(), inputPrefixes[i].c_str(), caseSensitive)) {

      // Found it !
      //*((*pi)[i].found) = true;
      if (checkPrivKey(addr[3], key, incr4, endomorphism, mode)) {
        nbFoundKey++;
        patternFound[i] = true;
        updateFound();
      }

    }

  }


}

void VanitySearch::checkAddr(int prefIdx, uint8_t *hash160, Int &key, int32_t incr, int endomorphism, bool mode) {

  if (hasPattern) {

    // Wildcard search
    string addr = secp->GetAddress(searchType, mode, hash160);

    for (int i = 0; i < (int)inputPrefixes.size(); i++) {

      if (Wildcard::match(addr.c_str(), inputPrefixes[i].c_str(), caseSensitive)) {

        // Found it !
        //*((*pi)[i].found) = true;
        if (checkPrivKey(addr, key, incr, endomorphism, mode)) {
          nbFoundKey++;
          patternFound[i] = true;
          updateFound();
        }

      }

    }

    return;

  }

  vector<PREFIX_ITEM> *pi = prefixes[prefIdx].items;

  if (onlyFull) {

    // Full addresses
    for (int i = 0; i < (int)pi->size(); i++) {

      if (stopWhenFound && *((*pi)[i].found))
        continue;

      if (ripemd160_comp_hash((*pi)[i].hash160, hash160)) {

        // Found it !
        *((*pi)[i].found) = true;
        // You believe it ?
        if (checkPrivKey(secp->GetAddress(searchType, mode, hash160), key, incr, endomorphism, mode)) {
          nbFoundKey++;
          updateFound();
        }

      }

    }

  } else {


    char a[64];

    string addr = secp->GetAddress(searchType, mode, hash160);

    for (int i = 0; i < (int)pi->size(); i++) {

      if (stopWhenFound && *((*pi)[i].found))
        continue;

      strncpy(a, addr.c_str(), (*pi)[i].prefixLength);
      a[(*pi)[i].prefixLength] = 0;

      if (strcmp((*pi)[i].prefix, a) == 0) {

        // Found it !
        *((*pi)[i].found) = true;
        if (checkPrivKey(addr, key, incr, endomorphism, mode)) {
          nbFoundKey++;
          updateFound();
        }

      }

    }

  }

}

// ----------------------------------------------------------------------------

#ifdef WIN64
DWORD WINAPI _FindKey(LPVOID lpParam) {
#else
void *_FindKey(void *lpParam) {
#endif
  TH_PARAM *p = (TH_PARAM *)lpParam;
  p->obj->FindKeyCPU(p);
  return 0;
}

#ifdef WIN64
DWORD WINAPI _FindKeyGPU(LPVOID lpParam) {
#else
void *_FindKeyGPU(void *lpParam) {
#endif
  TH_PARAM *p = (TH_PARAM *)lpParam;
  p->obj->FindKeyGPU(p);
  return 0;
}

// ----------------------------------------------------------------------------

void VanitySearch::checkAddresses(bool compressed, Int key, int i, Point p1) {

  unsigned char h0[20];
  Point pte1[1];
  Point pte2[1];

  // Point
  secp->GetHash160(searchType,compressed, p1, h0);
  prefix_t pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, i, 0, compressed);

  // Endomorphism #1
  pte1[0].x.ModMulK1(&p1.x, &beta);
  pte1[0].y.Set(&p1.y);

  secp->GetHash160(searchType, compressed, pte1[0], h0);

  pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, i, 1, compressed);

  // Endomorphism #2
  pte2[0].x.ModMulK1(&p1.x, &beta2);
  pte2[0].y.Set(&p1.y);

  secp->GetHash160(searchType, compressed, pte2[0], h0);

  pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, i, 2, compressed);

  // Curve symetrie
  // if (x,y) = k*G, then (x, -y) is -k*G
  p1.y.ModNeg();
  secp->GetHash160(searchType, compressed, p1, h0);
  pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, -i, 0, compressed);

  // Endomorphism #1
  pte1[0].y.ModNeg();

  secp->GetHash160(searchType, compressed, pte1[0], h0);

  pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, -i, 1, compressed);

  // Endomorphism #2
  pte2[0].y.ModNeg();

  secp->GetHash160(searchType, compressed, pte2[0], h0);

  pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, -i, 2, compressed);

}

// ----------------------------------------------------------------------------

void VanitySearch::checkAddressesSSE(bool compressed,Int key, int i, Point p1, Point p2, Point p3, Point p4) {

  unsigned char h0[20];
  unsigned char h1[20];
  unsigned char h2[20];
  unsigned char h3[20];
  Point pte1[4];
  Point pte2[4];
  prefix_t pr0;
  prefix_t pr1;
  prefix_t pr2;
  prefix_t pr3;

  // Point -------------------------------------------------------------------------
  secp->GetHash160(searchType, compressed, p1, p2, p3, p4, h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, i, 0, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, i + 1, 0, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, i + 2, 0, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, i + 3, 0, compressed);

  } else {

    checkAddrSSE(h0,h1,h2,h3,i,i+1,i+2,i+3,key,0,compressed);

  }

  // Endomorphism #1
  // if (x, y) = k * G, then (beta*x, y) = lambda*k*G
  pte1[0].x.ModMulK1(&p1.x, &beta);
  pte1[0].y.Set(&p1.y);
  pte1[1].x.ModMulK1(&p2.x, &beta);
  pte1[1].y.Set(&p2.y);
  pte1[2].x.ModMulK1(&p3.x, &beta);
  pte1[2].y.Set(&p3.y);
  pte1[3].x.ModMulK1(&p4.x, &beta);
  pte1[3].y.Set(&p4.y);

  secp->GetHash160(searchType, compressed, pte1[0], pte1[1], pte1[2], pte1[3], h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, i, 1, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, (i + 1), 1, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, (i + 2), 1, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, (i + 3), 1, compressed);

  } else {

    checkAddrSSE(h0, h1, h2, h3, i, i + 1, i + 2, i + 3, key, 1, compressed);

  }

  // Endomorphism #2
  // if (x, y) = k * G, then (beta2*x, y) = lambda2*k*G
  pte2[0].x.ModMulK1(&p1.x, &beta2);
  pte2[0].y.Set(&p1.y);
  pte2[1].x.ModMulK1(&p2.x, &beta2);
  pte2[1].y.Set(&p2.y);
  pte2[2].x.ModMulK1(&p3.x, &beta2);
  pte2[2].y.Set(&p3.y);
  pte2[3].x.ModMulK1(&p4.x, &beta2);
  pte2[3].y.Set(&p4.y);

  secp->GetHash160(searchType, compressed, pte2[0], pte2[1], pte2[2], pte2[3], h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, i, 2, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, (i + 1), 2, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, (i + 2), 2, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, (i + 3), 2, compressed);

  } else {

    checkAddrSSE(h0, h1, h2, h3, i, i + 1, i + 2, i + 3, key, 2, compressed);

  }

  // Curve symetrie -------------------------------------------------------------------------
  // if (x,y) = k*G, then (x, -y) is -k*G

  p1.y.ModNeg();
  p2.y.ModNeg();
  p3.y.ModNeg();
  p4.y.ModNeg();

  secp->GetHash160(searchType, compressed, p1, p2, p3, p4, h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, -i, 0, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, -(i + 1), 0, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, -(i + 2), 0, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, -(i + 3), 0, compressed);

  } else {

    checkAddrSSE(h0, h1, h2, h3, -i, -(i + 1), -(i + 2), -(i + 3), key, 0, compressed);

  }

  // Endomorphism #1
  // if (x, y) = k * G, then (beta*x, y) = lambda*k*G
  pte1[0].y.ModNeg();
  pte1[1].y.ModNeg();
  pte1[2].y.ModNeg();
  pte1[3].y.ModNeg();


  secp->GetHash160(searchType, compressed, pte1[0], pte1[1], pte1[2], pte1[3], h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, -i, 1, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, -(i + 1), 1, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, -(i + 2), 1, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, -(i + 3), 1, compressed);

  } else {

    checkAddrSSE(h0, h1, h2, h3, -i, -(i + 1), -(i + 2), -(i + 3), key, 1, compressed);

  }

  // Endomorphism #2
  // if (x, y) = k * G, then (beta2*x, y) = lambda2*k*G
  pte2[0].y.ModNeg();
  pte2[1].y.ModNeg();
  pte2[2].y.ModNeg();
  pte2[3].y.ModNeg();

  secp->GetHash160(searchType, compressed, pte2[0], pte2[1], pte2[2], pte2[3], h0, h1, h2, h3);

  if (!hasPattern) {

    pr0 = *(prefix_t *)h0;
    pr1 = *(prefix_t *)h1;
    pr2 = *(prefix_t *)h2;
    pr3 = *(prefix_t *)h3;

    if (prefixes[pr0].items)
      checkAddr(pr0, h0, key, -i, 2, compressed);
    if (prefixes[pr1].items)
      checkAddr(pr1, h1, key, -(i + 1), 2, compressed);
    if (prefixes[pr2].items)
      checkAddr(pr2, h2, key, -(i + 2), 2, compressed);
    if (prefixes[pr3].items)
      checkAddr(pr3, h3, key, -(i + 3), 2, compressed);

  } else {

    checkAddrSSE(h0, h1, h2, h3, -i, -(i + 1), -(i + 2), -(i + 3), key, 2, compressed);

  }

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void VanitySearch::getCPUStartingKey(int thId,Int& key,Point& startP) {
  if (rekey > 0) {
    // --- 在指定范围内生成随机私钥 ---
    if (!key.RandRange(min_range, max_range, secp->order)) {
        fprintf(stderr, "Fatal Error: CPU Thread %d failed to generate random key in range. Exiting...\n", thId);
        endOfSearch = true;
        return;
    }
    // --- 结束 ---
  } else {
    // 已有逻辑：基于 startKey + offset，用于确定性搜索模式
    key.Set(&startKey);
    // CPU Thread offset = thId * CPU_GRP_SIZE
    // FindKeyCPU iterates i from 0 to CPU_GRP_SIZE-1 relative to this key.
    Int off((uint64_t)thId); // Offset based on thread ID
    off.Mult((uint64_t)CPU_GRP_SIZE);
    key.Add(&off);
  }

  // 计算起始点 startP。
  // startP 是当前线程组搜索的第一个点所对应的密钥。
  // FindKeyCPU 中，点 pts[i] 对应私钥 (key + i).
  // pts[0] 对应私钥 key.
  // 所以 startP 应该是 key * G (+ startPubKey)
  // 原来的代码计算 startP 是基于 key + CPU_GRP_SIZE / 2，这与 pts[0] 对应的私钥 (key) 不一致。
  // pts[CPU_GRP_SIZE/2] 对应的私钥是 key + CPU_GRP_SIZE/2。
  // 需要确认 pts 数组中点和私钥的对应关系，以及 startP 到底代表哪个点。

  // 根据FindKeyCPU中的点生成逻辑 (startP = secp->DoubleDirect(Gn[CPU_GRP_SIZE/2-1]); ... startP = secp->AddDirect(pts[CPU_GRP_SIZE/2-1], Gn[CPU_GRP_SIZE/2-1]); _2Gn = startP; startP = Gn[CPU_GRP_SIZE/2-1])
  // 以及 pts[CPU_GRP_SIZE/2] = startP，和 pts[0] 的计算方式，
  // startP 在FindKeyCPU中被用作中心点 key + CPU_GRP_SIZE/2 的点。
  // 那么 getCPUStartingKey 中的 key 也应该用于计算 key + CPU_GRP_SIZE/2 对应的点。
  // Int km(&key);
  // km.Add((uint64_t)CPU_GRP_SIZE / 2); // 计算 key + CPU_GRP_SIZE / 2
  // startP = secp->ComputePublicKey(&km); // 计算 (key + CPU_GRP_SIZE / 2) * G

  // 但是 checkPrivKey 是用 key + incr_param 来计算私钥的，
  // 且 incr_param 是 pts 的索引 i (0 to CPU_GRP_SIZE-1) 或 -i。
  // pts[i] 对应的私钥是 (key + i).
  // 所以 getCPUStartingKey 生成的 key 应该是线程的基准私钥 (group_base_key)。
  // startP 应该对应于 pts[0]，也就是 key * G。

  // 让我们假设 key 就是线程的 group_base_key，而 startP 对应 key * G.
  // 在 FindKeyCPU 中，pts 数组应该通过 key * G 和 G 计算出来。
  // 但 FindKeyCPU 看起来是通过 startP (key + CPU_GRP_SIZE/2)*G 和 G 相关的点来计算 pts 的。
  // pts[0] = startP - (CPU_GRP_SIZE/2)*G = (key + CPU_GRP_SIZE/2)*G - (CPU_GRP_SIZE/2)*G = key * G.
  // 这个逻辑是对的：pts[0] 对应私钥 key.

  // 所以，getCPUStartingKey 应该计算 pts[0] 对应的点 (key * G)。
  // 并生成下一个组的起始密钥 (key + CPU_GRP_SIZE)。
  // startP 在 FindKeyCPU 中用于组内点计算的中心点，是 key + CPU_GRP_SIZE / 2 对应的点。
  // getCPUStartingKey 应该计算 startP，并在 FindKeyCPU 循环结束时更新 key 为下一个组的起始密钥。

  // 修正 getCPUStartingKey 逻辑：
  // 生成线程的基准密钥 key
  // 计算组的中心点 startP = (key + CPU_GRP_SIZE/2) * G
  Int key_for_startP(&key);
  key_for_startP.Add((uint64_t)CPU_GRP_SIZE / 2); // key_for_startP = key + CPU_GRP_SIZE/2
  startP = secp->ComputePublicKey(&key_for_startP); // startP = (key + CPU_GRP_SIZE/2) * G

  if(startPubKeySpecified)
      startP = secp->AddDirect(startP,startPubKey); // startP = (key + CPU_GRP_SIZE/2) * G + startPubKey

/*  // !!! 在这里添加 DEBUG 输出 !!!
  if (rekey > 0) {
      printf("DEBUG: CPU Thread %d Rekey - Generated Base Key: 0x%s (Range 0x%s to 0x%s)\n",
             thId, key.GetBase16().c_str(), min_range.GetBase16().c_str(), max_range.GetBase16().c_str());
  } else {
      printf("DEBUG: CPU Thread %d Deterministic - Base Key: 0x%s\n",
           thId, key.GetBase16().c_str());
  }
*/
  // 在 FindKeyCPU 循环结束时，需要更新 key 为下一个组的基准密钥 (key + CPU_GRP_SIZE)
  // 当前 key 已经是当前组的基准密钥了。循环结束后，它会被 key.Add((uint64_t)CPU_GRP_SIZE) 更新。

} // <--- 这是 getCPUStartingKey 函数的结

void VanitySearch::FindKeyCPU(TH_PARAM *ph) {

  // Global init
  int thId = ph->threadId;
  counters[thId] = 0;

  // CPU Thread
  // 確保 IntGroup 在函數退出時被銷毀
  IntGroup *grp = new IntGroup(CPU_GRP_SIZE/2+1);

  // Group Init
  Int  key; // This is the base key for the current group
  Point startP;
  // getCPUStartingKey sets the initial 'key' and calculates the initial 'startP'
  getCPUStartingKey(thId, key, startP);

  Int dx[CPU_GRP_SIZE/2+1];
  Point pts[CPU_GRP_SIZE];

  Int dy;
  Int dyn;
  Int _s;
  Int _p;
  Point pp;
  Point pn;
  grp->Set(dx);

  ph->hasStarted = true;
  ph->rekeyRequest = false;
  ph->isRunning = true; // 顯式設置為 true

  // Debug: Print initial info for this thread
  //printf("Debug CPU Thread %d: Starting search from key %s\n", thId, key.GetBase16().c_str());
  //printf("Debug CPU Thread %d: Target range [%s, %s]\n", thId, min_range.GetBase16().c_str(), max_range.GetBase16().c_str());


  // >>> 主循环: 检查全局停止标志 <<<
  while (!endOfSearch && ph->isRunning) { // 同时检查全局停止标志和线程自身的运行标志 (用于确定性模式范围结束)

        // >>> 添加：检查 rekey 请求 <<<
        if (ph->rekeyRequest) {
            // Debug: printf("Debug CPU Thread %d: Rekey request received.\n", thId);
            getCPUStartingKey(thId, key, startP); // 重新生成 key 和 startP
            ph->rekeyRequest = false; // 重置请求标志
        }

        // --- Add deterministic mode stop check BEFORE processing the group ---
        // If deterministic mode and the current group's base key is already beyond max_range, stop this thread.
        // This check should happen *before* processing the group using the 'key' that was set in the previous iteration
        // or by getCPUStartingKey.
        Int next_group_start_check(&key);
        // We check if the *start* of the group we *are about to process* is already out of range.
        // If key is the start of the current group, we check if key > max_range.
        // If key becomes the start of the *next* group *after* the loop body, the check should be on the key *before* it's updated.
        // Let's stick to checking 'key' as the start of the *current* group.
        if (rekey == 0 && key.IsGreater(&max_range)) {
             // Debug: printf("Debug CPU Thread %d: Current group starts at %s, which is > max_range %s. Setting isRunning=false.\n",
             //       thId, key.GetBase16().c_str(), max_range.GetBase16().c_str());
             ph->isRunning = false; // Signal this thread is done
             break; // Exit the main loop
        }
        // --- End deterministic mode stop check ---


    // Fill group (calculate pts based on startP and Gn)
    int i;
    int hLength = (CPU_GRP_SIZE / 2 - 1);

    for (i = 0; i < hLength; i++) {
      dx[i].ModSub(&Gn[i].x, &startP.x);
    }
    dx[i].ModSub(&Gn[i].x, &startP.x);  // For the first point (pts[0] corresponding to startP - (CPU_GRP_SIZE/2)*G)
    dx[i+1].ModSub(&_2Gn.x, &startP.x); // For the next center point (corresponding to startP + (CPU_GRP_SIZE/2)*G)


    // Grouped ModInv
    grp->ModInv();

    // We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
    // We compute key in the positive and negative way from the center of the group

    // center point
    // pts[CPU_GRP_SIZE/2] corresponds to (key + CPU_GRP_SIZE/2) * G + startPubKey
    pts[CPU_GRP_SIZE/2] = startP;

    for (i = 0; i<hLength ; i++) { // Removed && !endOfSearch from here, check inside the inner loop

      // >>> 添加内部检查：如果在计算点的过程中全局停止，提前退出 <<<
      if (endOfSearch || !ph->isRunning) { // 检查全局和线程自身标志
          // Debug: printf("Debug CPU Thread %d: Found endOfSearch/!isRunning during point calculation (i=%d).\n", thId, i);
          break; // 退出点计算循环
      }
      // >>> 结束添加 <<<

      pp = startP;
      pn = startP;

      // P = startP + (i+1)*G  (corresponds to keys key + CPU_GRP_SIZE/2 + (i+1))
      dy.ModSub(&Gn[i].y,&pp.y);

      _s.ModMulK1(&dy, &dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
      _p.ModSquareK1(&_s);            // _p = pow2(s)

      pp.x.ModNeg();
      pp.x.ModAdd(&_p);
      pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

      pp.y.ModSub(&Gn[i].x, &pp.x);
      pp.y.ModMulK1(&_s);
      pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);

      // P = startP - (i+1)*G  , if (x,y) = (i+1)*G then (x,-y) = -(i+1)*G
      // corresponds to keys key + CPU_GRP_SIZE/2 - (i+1)
      dyn.Set(&Gn[i].y);
      dyn.ModNeg();
      dyn.ModSub(&pn.y);

      _s.ModMulK1(&dyn, &dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
      _p.ModSquareK1(&_s);            // _p = pow2(s)

      pn.x.ModNeg();
      pn.x.ModAdd(&_p);
      pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

      pn.y.ModSub(&Gn[i].x, &pn.x);
      pn.y.ModMulK1(&_s);
      pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);

      pts[CPU_GRP_SIZE/2 + (i+1)] = pp; // Indices from CPU_GRP_SIZE/2 + 1 to CPU_GRP_SIZE-1
      pts[CPU_GRP_SIZE/2 - (i+1)] = pn; // Indices from CPU_GRP_SIZE/2 - 1 to 0
    }

    // First point (pts[0] corresponding to startP - (CPU_GRP_SIZE/2)*G)
    // Check if the inner loop above exited early
    if (endOfSearch || !ph->isRunning) {
        // If point calculation was interrupted, skip address checking for this group
        // Debug: printf("Debug CPU Thread %d: Skipping address check due to early point calc exit.\n", thId);
        // Continue to the next iteration of the main while loop, which will re-check conditions
    } else {

        // Finish calculating pts[0]
        pn = startP;
        dyn.Set(&Gn[i].y); // Gn[i] here is Gn[CPU_GRP_SIZE/2-1] because i reached hLength (CPU_GRP_SIZE/2-1) in the loop
        dyn.ModNeg();
        dyn.ModSub(&pn.y);

        _s.ModMulK1(&dyn, &dx[i]); // dx[i] here is dx[CPU_GRP_SIZE/2-1]
        _p.ModSquareK1(&_s);

        pn.x.ModNeg();
        pn.x.ModAdd(&_p);
        pn.x.ModSub(&Gn[i].x);

        pn.y.ModSub(&Gn[i].x, &pn.x);
        pn.y.ModMulK1(&_s);
        pn.y.ModAdd(&Gn[i].y);

        pts[0] = pn;

        // Next start point calculation (startP + GRP_SIZE*G)
        // startP should be updated to the center point of the *next* group
        // The next center point is current startP + (CPU_GRP_SIZE/2)*G + (CPU_GRP_SIZE/2)*G = startP + CPU_GRP_SIZE*G
        // And pts[CPU_GRP_SIZE] should be this point? No, the next startP is based on dx[i+1] which used _2Gn (CPU_GRP_SIZE * G)
        // The calculation below looks like it computes pts[CPU_GRP_SIZE], let's re-verify the original intent.
        // Original comment: "_2Gn = CPU_GRP_SIZE*G", "startP = secp->DoubleDirect(Gn[CPU_GRP_SIZE/2-1]);" (this sets _2Gn?) No, the comment is wrong.
        // Let's trace: _2Gn is calculated *once* in VanitySearch constructor as CPU_GRP_SIZE*G.
        // In the loop: dx[i+1].ModSub(&_2Gn.x, &startP.x); -> This suggests we are computing the line between startP and startP + CPU_GRP_SIZE*G (_2Gn).
        // The resulting point 'pp' seems to be startP + CPU_GRP_SIZE*G. This is the correct center for the *next* group.
        // Original code seems to correctly update startP for the next iteration here.

        pp = startP; // Start from the current center point
        dy.ModSub(&_2Gn.y, &pp.y); // Vector from current center to (current center + CPU_GRP_SIZE*G)

        _s.ModMulK1(&dy, &dx[i+1]); // dx[i+1] uses _2Gn, so it's ModInv( (_2Gn.x - startP.x) )
        _p.ModSquareK1(&_s);

        pp.x.ModNeg();
        pp.x.ModAdd(&_p);
        pp.x.ModSub(&_2Gn.x); // Should be startP.x + (startP + CPU_GRP_SIZE*G).x ? No, check point addition formula.
                             // R = P + Q. s = (Q.y-P.y)/(Q.x-P.x). Rx = s^2 - P.x - Q.x. Ry = Q.y + s*(Rx - Q.x).
                             // Here P=startP, Q=_2Gn (relative to origin). For startP + _2Gn, P=startP, Q=_2Gn.
                             // The calculation looks right for R = startP + _2Gn.
                             // R.x = s^2 - startP.x - _2Gn.x
                             // R.y = _2Gn.y + s*(R.x - _2Gn.x)

        // The original code for Ry is: pp.y.ModSub(&_2Gn.x, &pp.x); pp.y.ModMulK1(&_s); pp.y.ModSub(&_2Gn.y);
        // Let's rewrite the formula: Ry = s * (Q.x - R.x) - Q.y
        // Here Q is _2Gn. So Ry = s * (_2Gn.x - R.x) - _2Gn.y.
        // This matches the original code's calculation of 'pp.y'.
        // So 'pp' becomes startP + CPU_GRP_SIZE * G. This is the correct 'startP' for the *next* group.

        pp.y.ModSub(&_2Gn.x, &pp.x);
        pp.y.ModMulK1(&_s);
        pp.y.ModSub(&_2Gn.y);
        startP = pp; // Update startP for the next iteration

        // Check addresses
        if (useSSE) {
            // SSE 部分的代码
            for (int i = 0; i < CPU_GRP_SIZE; i += 4) { // Note: Still need to check endOfSearch inside this loop

                 // >>> 添加内部检查 <<<
                 if (endOfSearch || !ph->isRunning) { // 检查全局和线程自身标志
                     // Debug: printf("Debug CPU Thread %d: Found endOfSearch/!isRunning during SSE batch (i=%d).\n", thId, i);
                     break; // 退出当前批次循环
                 }
                 // >>> 结束添加 <<<

                 // Calculate the actual keys being checked in this SSE group
                 // key is the base key for the current group (pts[0] corresponds to key)
                 // pts[i] corresponds to key + i
                 // Int current_key_base(&key); // Not strictly needed, can work with key directly

                 // Debug: Print the keys being checked in this SSE block (Optional)
                 // Int key1(key); key1.Add((uint64_t)i);
                 // Int key2(key); key2.Add((uint64_t)(i+1));
                 // Int key3(key); key3.Add((uint64_t)(i+2));
                 // Int key4(key); key4.Add((uint64_t)(i+3));
                 // printf("Debug CPU Thread %d SSE Block (i=%d): Checking keys %s, %s, %s, %s against range [%s, %s]\n",
                 //        ph->threadId, i,
                 //        key1.GetBase16().c_str(), key2.GetBase16().c_str(),
                 //        key3.GetBase16().c_str(), key4.GetBase16().c_str(),
                 //       min_range.GetBase16().c_str(), max_range.GetBase16().c_str());


                 switch (searchMode) {
                   case SEARCH_COMPRESSED:
                     checkAddressesSSE(true, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
                     break;
                   case SEARCH_UNCOMPRESSED:
                     checkAddressesSSE(false, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
                     break;
                   case SEARCH_BOTH:
                     checkAddressesSSE(true, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
                     if (endOfSearch || !ph->isRunning) break; // Optional: Check again between calls if both modes are searched
                     checkAddressesSSE(false, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
                     break;
                 } // end switch

             } // end for i += 4 (SSE batch loop)

        } else { // Non-SSE 部分的代码 (就是你提供的那个 snippet 所在的部分)

             for (int i = 0; i < CPU_GRP_SIZE; i ++) { // Note: Still need to check endOfSearch inside this loop

                 // >>> 添加内部检查 <<<
                 if (endOfSearch || !ph->isRunning) { // 检查全局和线程自身标志
                     // Debug: printf("Debug CPU Thread %d: Found endOfSearch/!isRunning during Non-SSE batch (i=%d).\n", thId, i);
                     break; // 退出当前批次循环
                 }
                 // >>> 结束添加 <<<

                 // Calculate the actual key being checked
                 // key is the base key for the current group (pts[0] corresponds to key)
                 // pts[i] corresponds to key + i
                 // Int current_key(&key); // Not strictly needed
                 // current_key.Add((uint64_t)i); // This is the actual key being considered

                 // Debug: Print the key being checked (Optional)
                 // printf("Debug CPU Thread %d (i=%d): Checking key %s against range [%s, %s]\n",
                 //        ph->threadId, i,
                 //        current_key.GetBase16().c_str(),
                 //        min_range.GetBase16().c_str(), max_range.GetBase16().c_str());

                 switch (searchMode) {
                 case SEARCH_COMPRESSED:
                   checkAddresses(true, key, i, pts[i]);
                   break;
                 case SEARCH_UNCOMPRESSED:
                   checkAddresses(false, key, i, pts[i]);
                   break;
                 case SEARCH_BOTH:
                   checkAddresses(true, key, i, pts[i]);
                   if (endOfSearch || !ph->isRunning) break; // Optional: Check again between calls
                   checkAddresses(false, key, i, pts[i]);
                   break;
                 } // end switch

             } // end for i++ (Non-SSE batch loop)

        } // end if (useSSE) else

        // 在批次处理循环结束后，再次检查 endOfSearch 或 !ph->isRunning。
        // 如果内层循环因为这些标志提前退出，这里的 check 会是 false，不会执行后面的 key.Add 和 counters 逻辑
        // 并会跳到外层 while 循环的条件检查，从而可能退出线程。
        // if (endOfSearch || !ph->isRunning) {
        //     Debug: printf("Debug CPU Thread %d: Batch processing exited due to endOfSearch/!isRunning.\n", thId);
        // }


        // Update key to the base key of the *next* group
        // This only happens if the batch processing completed without hitting endOfSearch / !ph->isRunning
        key.Add((uint64_t)CPU_GRP_SIZE);
        counters[thId]+= 6ULL * CPU_GRP_SIZE; // Point + endo #1 + endo #2 + Symetric point + endo #1 + endo #2

        // --- 原有的确定性模式范围检查 ---
        // 检查下一组的起始 key 是否超出了范围
        // 这个检查放在这里是因为 key 刚刚被更新为下一组的起始值
        if (rekey == 0) {
            // key is already the start of the next group here
            if (key.IsGreater(&max_range)) {
                 // Debug: printf("Debug CPU Thread %d: Next group starts at %s, which is > max_range %s. Setting isRunning=false.\n",
                 //       thId, key.GetBase16().c_str(), max_range.GetBase16().c_str());
                 ph->isRunning = false; // Signal this thread is done its range
                 // No need for break here, the main while loop condition (!endOfSearch && ph->isRunning) will catch it
                 // in the next iteration. However, adding break makes it exit immediately. Let's add break for faster exit.
                 break; // Exit the main thread loop
            }
        }
        // --- 结束范围检查 ---

    } // end if (!(endOfSearch || !ph->isRunning)) after point calculation block


  } // end while (!endOfSearch && ph->isRunning) (Main thread loop)


  // >>> 在 while 循环结束后，确保设置 isRunning 为 false <<<
  // 无论循环是因为 endOfSearch, !ph->isRunning, 或确定性范围结束而退出，都确保这个标志被设置
  ph->isRunning = false;
  // Debug: printf("Debug CPU Thread %d: Exiting FindKeyCPU function. isRunning set to false.\n", thId);

  // Clean up allocated memory
  delete grp; // Delete the IntGroup object

  // Note: The Point and Int arrays (p, keys) for GPU threads are deleted in FindKeyGPU.
  // The Point arrays (Gn, _2Gn) are global.
  // The Int array (dx), Point array (pts) and Ints (dy, dyn, _s, _p) are local to the function and will be cleaned up automatically.
  // The parameters 'ph' are allocated in Search() and freed there.

} // end VanitySearch::FindKeyCPU
// ----------------------------------------------------------------------------
void VanitySearch::getGPUStartingKeys(int thId, int groupSize, int nbThread, Int *keys, Point *p) {
    // thId is the GPU instance ID (0, 1, 2... if using multiple GPUs)
    // nbThread is the number of threads *launched on this specific GPU instance*

    for (int i = 0; i < nbThread; i++) {
        if (rekey > 0) {
            // --- 在指定范围内生成随机私钥 (Random mode) ---
            // RandRange 应该在这个范围内生成 key
            if (!keys[i].RandRange(min_range, max_range, secp->order)) {
                // 如果生成失败，说明范围有问题或 RandRange 有bug，需要致命退出
                fprintf(stderr, "Fatal Error: GPU Instance %d, Thread %d failed to generate random key in range [0x%s, 0x%s]. Exiting...\n",
                        thId, i, min_range.GetBase16().c_str(), max_range.GetBase16().c_str());
                endOfSearch = true; // 通知所有线程退出
                return; // 返回，此线程将结束
            }
            // --- 添加简单调式打印：显示随机生成的基线私钥 ---
            // 打印到 stderr 避免干擾主狀態輸出
       //     fprintf(stderr, "DEBUG GPU Instance %d, Thread %d: Random Base Key generated: 0x%s (Range [0x%s, 0x%s])\n",
         //           thId, i, keys[i].GetBase16().c_str(), min_range.GetBase16().c_str(), max_range.GetBase16().c_str());
            // --- 结束简单调式打印 ---

        } else {
            // 已有逻辑：基于 startKey + offset (Deterministic mode)
            keys[i].Set(&startKey);
            // Original GPU offset logic (preserved for rekey == 0):
            // Note: This offset logic seems to create large, non-contiguous jumps for threads/GPUs
            Int offT((uint64_t)i); // Thread index within this GPU instance's total threads
            offT.ShiftL(80); // Original large shifts
            Int offG((uint64_t)thId); // GPU instance ID
            offG.ShiftL(112); // Original large shifts
            keys[i].Add(&offT);
            keys[i].Add(&offG);
            // --- Optional: Add simple debug print for deterministic GPU start keys if needed ---
            // fprintf(stderr, "DEBUG GPU Instance %d, Thread %d: Deterministic Base Key: 0x%s\n", thId, i, keys[i].GetBase16().c_str());
            // --- End optional debug print ---
        }
        // --- 原始逻辑：计算组的起始点 ---
        // Starting key is at the middle of the group (STEP_SIZE is used by GPU Kernel iteration)
        Int k(keys + i);
        k.Add((uint64_t)(STEP_SIZE / 2)); // Use STEP_SIZE instead of GRP_SIZE for GPU
        p[i] = secp->ComputePublicKey(&k);
        if (startPubKeySpecified)
          p[i] = secp->AddDirect(p[i], startPubKey);
    }
}


void VanitySearch::FindKeyGPU(TH_PARAM *ph) {

  bool ok = true;

#ifdef WITHGPU

  // Global init
  int thId = ph->threadId;
  GPUEngine g(ph->gridSizeX,ph->gridSizeY, ph->gpuId, maxFound, (rekey!=0));
  int nbThread = g.GetNbThread();
  Point *p = new Point[nbThread];
  Int *keys = new Int[nbThread];
  vector<ITEM> found;

  printf("❀  GPU: %s\n",g.deviceName.c_str());

  counters[thId] = 0;

  getGPUStartingKeys(thId, g.GetGroupSize(), nbThread, keys, p);

  g.SetSearchMode(searchMode);
  g.SetSearchType(searchType);
  if (onlyFull) {
    g.SetPrefix(usedPrefixL,nbPrefix);
  } else {
    if(hasPattern)
      g.SetPattern(inputPrefixes[0].c_str());
    else
      g.SetPrefix(usedPrefix);
  }

  getGPUStartingKeys(thId, g.GetGroupSize(), nbThread, keys, p);
  ok = g.SetKeys(p);
  ph->rekeyRequest = false;

  ph->hasStarted = true;

  // GPU Thread
  while (ok && !endOfSearch) {

    if (ph->rekeyRequest) {
      getGPUStartingKeys(thId, g.GetGroupSize(), nbThread, keys, p);
      ok = g.SetKeys(p);
      ph->rekeyRequest = false;
    }

    // Call kernel
    ok = g.Launch(found);

    for(int i=0;i<(int)found.size() && !endOfSearch;i++) {

      ITEM it = found[i];
      checkAddr(*(prefix_t *)(it.hash), it.hash, keys[it.thId], it.incr, it.endo, it.mode);

    }

    if (ok) {
      for (int i = 0; i < nbThread; i++) {
        keys[i].Add((uint64_t)STEP_SIZE);
      }
      counters[thId] += 6ULL * STEP_SIZE * nbThread; // Point +  endo1 + endo2 + symetrics
    }

  }

  delete[] keys;
  delete[] p;

#else
  ph->hasStarted = true;
  printf("GPU code not compiled, use -DWITHGPU when compiling.\n");
#endif

  ph->isRunning = false;

}

// ----------------------------------------------------------------------------

bool VanitySearch::isAlive(TH_PARAM *p) {

  bool isAlive = true;
  int total = nbCPUThread + nbGPUThread;
  for(int i=0;i<total;i++)
    isAlive = isAlive && p[i].isRunning;

  return isAlive;

}

// ----------------------------------------------------------------------------

bool VanitySearch::hasStarted(TH_PARAM *p) {

  bool hasStarted = true;
  int total = nbCPUThread + nbGPUThread;
  for (int i = 0; i < total; i++)
    hasStarted = hasStarted && p[i].hasStarted;

  return hasStarted;

}

// ----------------------------------------------------------------------------

void VanitySearch::rekeyRequest(TH_PARAM *p) {

  bool hasStarted = true;
  int total = nbCPUThread + nbGPUThread;
  for (int i = 0; i < total; i++)
  p[i].rekeyRequest = true;

}

// ----------------------------------------------------------------------------

uint64_t VanitySearch::getGPUCount() {

  uint64_t count = 0;
  for(int i=0;i<nbGPUThread;i++)
    count += counters[0x80L+i];
  return count;

}

uint64_t VanitySearch::getCPUCount() {

  uint64_t count = 0;
  for(int i=0;i<nbCPUThread;i++)
    count += counters[i];
  return count;

}

// ----------------------------------------------------------------------------

void VanitySearch::Search(int nbThread,std::vector<int> gpuId,std::vector<int> gridSize) {

  double t0;
  double t1;
  endOfSearch = false;
  nbCPUThread = nbThread;
  nbGPUThread = (useGpu?(int)gpuId.size():0);
  nbFoundKey = 0;

  memset(counters,0,sizeof(counters));

  printf("❀  Number of CPU thread: %d\n", nbCPUThread);

  TH_PARAM *params = (TH_PARAM *)malloc((nbCPUThread + nbGPUThread) * sizeof(TH_PARAM));
  memset(params,0,(nbCPUThread + nbGPUThread) * sizeof(TH_PARAM));

  // Launch CPU threads
  for (int i = 0; i < nbCPUThread; i++) {
    params[i].obj = this;
    params[i].threadId = i;
    params[i].isRunning = true;

#ifdef WIN64
    DWORD thread_id;
    CreateThread(NULL, 0, _FindKey, (void*)(params+i), 0, &thread_id);
    ghMutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &_FindKey, (void*)(params+i));
    ghMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
  }

  // Launch GPU threads
  for (int i = 0; i < nbGPUThread; i++) {
    params[nbCPUThread+i].obj = this;
    params[nbCPUThread+i].threadId = 0x80L+i;
    params[nbCPUThread+i].isRunning = true;
    params[nbCPUThread+i].gpuId = gpuId[i];
    params[nbCPUThread+i].gridSizeX = gridSize[2*i];
    params[nbCPUThread+i].gridSizeY = gridSize[2*i+1];
#ifdef WIN64
    DWORD thread_id;
    CreateThread(NULL, 0, _FindKeyGPU, (void*)(params+(nbCPUThread+i)), 0, &thread_id);
#else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &_FindKeyGPU, (void*)(params+(nbCPUThread+i)));
#endif
  }

#ifndef WIN64
  setvbuf(stdout, NULL, _IONBF, 0);
#endif

  uint64_t lastCount = 0;
  uint64_t gpuCount = 0;
  uint64_t lastGPUCount = 0;

  // Key rate smoothing filter
  #define FILTER_SIZE 8
  double lastkeyRate[FILTER_SIZE];
  double lastGpukeyRate[FILTER_SIZE];
  uint32_t filterPos = 0;

  double keyRate = 0.0;
  double gpuKeyRate = 0.0;

  memset(lastkeyRate,0,sizeof(lastkeyRate));
  memset(lastGpukeyRate,0,sizeof(lastkeyRate));

  // Wait that all threads have started
  while (!hasStarted(params)) {
    Timer::SleepMillis(500);
  }

  t0 = Timer::get_tick();
  startTime = t0;

  while (isAlive(params)) {

    int delay = 2000;
    while (isAlive(params) && delay>0) {
      Timer::SleepMillis(500);
      delay -= 500;
    }

    gpuCount = getGPUCount();
    uint64_t count = getCPUCount() + getGPUCount(); // 当前总计key数

    t1 = Timer::get_tick();
    keyRate = (double)(count - lastCount) / (t1 - t0);
    gpuKeyRate = (double)(gpuCount - lastGPUCount) / (t1 - t0);
    lastkeyRate[filterPos%FILTER_SIZE] = keyRate;
    lastGpukeyRate[filterPos%FILTER_SIZE] = gpuKeyRate;
    filterPos++;

    // KeyRate smoothing
    double avgKeyRate = 0.0;
    double avgGpuKeyRate = 0.0;
    uint32_t nbSample;
    for (nbSample = 0; (nbSample < FILTER_SIZE) && (nbSample < filterPos); nbSample++) {
      avgKeyRate += lastkeyRate[nbSample];
      avgGpuKeyRate += lastGpukeyRate[nbSample];
    }
    avgKeyRate /= (double)(nbSample);
    avgGpuKeyRate /= (double)(nbSample);

    if (isAlive(params)) {
      // --- 状态打印语句 ---
      char status_buffer[512]; // 使用一个缓冲区来构建完整的状态字符串
      int written = snprintf(status_buffer, sizeof(status_buffer),
                             "\r❀  [%.2f Mkey/s][GPU %.2f Mkey/s][Total 2^%.2f]%s",
                             avgKeyRate / 1000000.0, avgGpuKeyRate / 1000000.0,
                             log2((double)count), GetExpectedTime(avgKeyRate, (double)count).c_str());

      // 如果开启了rekey模式 (-R > 0)，则添加rekey次数
      if (this->rekey > 0 && written < sizeof(status_buffer)) {
          written += snprintf(status_buffer + written, sizeof(status_buffer) - written,
                              "[Rekey %u]", this->rekeyCount); // 添加rekey次数
      }

      // 添加 Found 次数和末尾空格
      if (written < sizeof(status_buffer)) {
          snprintf(status_buffer + written, sizeof(status_buffer) - written,
                   "[Found %d]  ", nbFoundKey); // 添加 Found 次数和末尾空格
      }

      printf("%s", status_buffer); // 打印构建好的状态字符串
      // --- 结束 ---
    }

    if (rekey > 0) {
      if ((count - lastRekey) > (1000000 * rekey)) {
        // Rekey request
        rekeyRequest(params);
        lastRekey = count;
        this->rekeyCount++; // <-- 在这里增加rekey次数
      }
    }

    lastCount = count;
    lastGPUCount = gpuCount;
    t0 = t1;

  }

  free(params);

}
// ----------------------------------------------------------------------------

string VanitySearch::GetHex(vector<unsigned char> &buffer) {

  string ret;

  char tmp[128];
  for (int i = 0; i < (int)buffer.size(); i++) {
    sprintf(tmp,"%02X",buffer[i]);
    ret.append(tmp);
  }

  return ret;

}
