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

// add openssl
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/ripemd.h>
#define ECCTYPE "secp256k1"
//

// 添加这三行
#include <array>
#include <memory>
#include <stdexcept>

using namespace std;

Point Gn[CPU_GRP_SIZE / 2];
Point _2Gn;

// ----------------------------------------------------------------------------

VanitySearch::VanitySearch(Secp256K1 *secp, vector<std::string> &inputPrefixes, string seed, string start_key, int Random_bit, int FuncLevel, int searchMode,
	bool useGpu, bool stop, string outputFile, bool useSSE, uint32_t maxFound,
	uint64_t rekey, bool caseSensitive, Point &startPubKey, bool paranoiacSeed)
	:inputPrefixes(inputPrefixes) {

	this->secp = secp;
	this->searchMode = searchMode;
	this->useGpu = useGpu;
	this->stopWhenFound = stop;
	this->outputFile = outputFile;
	this->useSSE = useSSE;
	this->nbGPUThread = 0;
	this->maxFound = maxFound;
	this->rekey = rekey;
	this->searchType = -1;
	this->startPubKey = startPubKey;
	this->hasPattern = false;
	this->caseSensitive = caseSensitive;
	this->startPubKeySpecified = !startPubKey.isZero();
	this->Random_bits = Random_bit;
	this->FunctionLevel = FuncLevel;

	//
	bool all_algorithms_fl = false;// = true;
	bool screen_fl = false;// = true;
	bool start_seed_fl = false;// = true;
	keys_seed_fl = false;

	if (FunctionLevel >= 1){
		all_algorithms_fl = true;
	}
	if (FunctionLevel >= 2){
		screen_fl = true;
	}
	if (FunctionLevel >= 3){
		start_seed_fl = true;
	}
	if (FunctionLevel >= 4){
		keys_seed_fl = true;
	}

	// openssl
	if(all_algorithms_fl){
		OpenSSL_add_all_algorithms();// OpenSSL initialization code
	}
	//EVP_cleanup(); The not used //OpenSSL cleanup code
	//static EC_KEY *myecc = NULL;
	//myecc = EC_KEY_new_by_curve_name(OBJ_txt2nid(ECCTYPE));

	// Logo 2
/*#ifndef WIN64
    // 使用 popen 捕获 openssl version -v 的输出
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("openssl version -v", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // 在输出前加上 [🟑 ]
    if (!result.empty()) {
       printf(" \n");
       printf("[🟑 ]%s", result.c_str());
       
    }
#endif
*/       
        printf("[🟑 ]OpenSSL level %d\n", FunctionLevel);
	// Seed random number generator with performance counter
	if (start_seed_fl) {
             //RandAddSeed();
	}

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
			#ifdef WIN64
				it.prefix = _strdup(it.prefix); // We need to allocate here, subList will be destroyed
			#else
				it.prefix = strdup(it.prefix); // We need to allocate here, subList will be destroyed
			#endif
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
        printf("[🟑 ]Difficulty: %.0f\n", _difficulty);
        printf("Search: %s [%s, Case unsensitive] (Lookup size %d)\n", inputPrefixes[0].c_str(), seachInfo.c_str(), unique_sPrefix);
      } else {
        printf("[🟑 ]Difficulty: %.0f\n", _difficulty);
        printf("[🟑 ]Search: %s [%s]\n", inputPrefixes[0].c_str(), seachInfo.c_str());
      }
    } else {
      if (onlyFull) {
        printf("[🟑 ]Search: %d addresses (Lookup size %d,[%d,%d]) [%s]\n", nbPrefix, unique_sPrefix, minI, maxI, seachInfo.c_str());
      } else {
        printf("[🟑 ]Search: %d prefixes (Lookup size %d) [%s]\n", nbPrefix, unique_sPrefix, seachInfo.c_str());
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
      printf("Search: %s [%s]\n", inputPrefixes[0].c_str(), searchInfo.c_str());
    } else {
      printf("Search: %d patterns [%s]\n", (int)inputPrefixes.size(), searchInfo.c_str());
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
  // Disable endomorphism
  //beta.SetBase16("7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee");
  //lambda.SetBase16("5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72");
  //beta2.SetBase16("851695d49a83f8ef919bb86153cbcb16630fb68aed0a766a3ec693d68e6afa40");
  //lambda2.SetBase16("ac9c52b33fa3cf1f5ad9e3fd77ed9ba4a880b9fc8ec739c2e0cfc810b51283ce");

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
  //sha256(hseed, 64, (unsigned char *)startKey.bits64);

  //Set startKey from start_key
  startKey.Rand(Random_bits);// bits 66

  if (start_key.length() > 1) { //if (1) {
	  if (start_key.length() != 64) {
		  //printf("PrivKeyHex: Error invalid privkey specified (64 character length)\n");
		  printf("StartKeyHex: Error invalid privkey specified (64 character length)\r");
		  exit(-1);
	  }
	  for (int i = 0; i < 32; i++) {
		  unsigned char my1ch = 0;
		  sscanf(&start_key[2 * i], "%02X", &my1ch);
		  startKey.SetByte(31 - i, my1ch);
	  }
  }
  // end Set startKey

  char *ctimeBuff;
  time_t now = time(NULL);
  ctimeBuff = ctime(&now);
  printf("[🟑 ]Start %s", ctimeBuff);

  if (rekey > 0) {
    printf("[🟑 ]Base Key: Randomly changed every %.0f Mkeys\n",(double)rekey);
  } else {
    printf("[🟑 ]Base Key: %s\n", startKey.GetBase16().c_str());
  }

}

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
          printf("Ignoring prefix \"%s\" (Unreachable, 31h1 to 3R2c only)\r", prefix.c_str());
        return false;
      }
    }

    if (result.size() != 25) {
      printf("Ignoring prefix \"%s\" (Invalid size)\r", prefix.c_str());
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

void VanitySearch::output(string addr,string pAddr,string pAddrHex) {

#ifdef WIN64
   WaitForSingleObject(ghMutex,INFINITE);
#else
  pthread_mutex_lock(&ghMutex);
#endif

  FILE *f = stdout;
  bool needToClose = false;

  outputFile = "Found.txt";// Fix name

  if (outputFile.length() > 0) {
    f = fopen(outputFile.c_str(), "a+");//f = fopen(outputFile.c_str(), "a");
    if (f == NULL) {
      printf("Cannot open %s for writing\n", outputFile.c_str());
      f = stdout;
    } else {
      needToClose = true;
    }
  }

  if(!needToClose)
    printf("\n");

    fprintf(f, "Add:%s\n", addr.c_str());

  if (startPubKeySpecified) {

    fprintf(f, "Key:%s\n", pAddr.c_str());

  } else {

    switch (searchType) {
    case P2PKH:
      //fprintf(f, "Priv (WIF): p2pkh:%s\n", pAddr.c_str());
      break;
    case P2SH:
      //fprintf(f, "Priv (WIF): p2wpkh-p2sh:%s\n", pAddr.c_str());
      break;
    case BECH32:
      //fprintf(f, "Priv (WIF): p2wpkh:%s\n", pAddr.c_str());
      break;
    }
    fprintf(f, "Key:0x%s\n", pAddrHex.c_str());

  }

  if(needToClose)
    fclose(f);

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

bool VanitySearch::checkPrivKey(string addr, Int &key, int32_t incr, int endomorphism, bool mode) {

  Int k = key;
  Point sp = startPubKey;

  if (incr < 0) {
    k.Add((uint64_t)(-incr));
    k.Neg();
    k.Add(&secp->order);
    if (startPubKeySpecified) sp.y.ModNeg();
  } else {
    k.Add((uint64_t)incr);
  }

  // Check addresses
  Point p = secp->ComputePublicKey(&k);
  if (startPubKeySpecified) p = secp->AddDirect(p, sp);
  string chkAddr = secp->GetAddress(searchType, mode, p); // 生成的完整地址是 chkAddr
  // --- 修改后的调试输出 - 只保留以下两行 ---
  //printf("\nPrivate Key (HEX): %s\n", k.GetBase16().c_str());
  //printf("Target Address Prefix: %s\n", addr.c_str());
  // --- 调试输出结束 ---


  if (strncmp(chkAddr.c_str(), addr.c_str(), addr.length()) != 0) {
       printf("\n");
      //printf("\nWarning, wrong private key generated !\n");
      printf("[🟐 ]Add:%s\n", addr.c_str());
      //printf("[🟐 ]Check:%s\n", chkAddr.c_str());
      printf("[🟐 ]Key:%s \n", k.GetBase16().c_str());
      
      //printf("[🟐 ]Endo:%d incr:%d comp:%d\n", endomorphism, incr, mode);
      //  return false;  <---  确保这里没有 return false;

  } else {
    output(chkAddr, secp->GetPrivAddress(mode ,k), k.GetBase16()); // **使用 chkAddr 替代 addr，写入完整地址**
    // Timer::SleepMillis(1000); // 移除不必要的延迟
    // Timer::SleepMillis(500);  // 移除不必要的延迟
    //printf("  Addr :%s\n", addr.c_str());
    printf("\n");
    printf("[🟐 ]Add:%s\n", chkAddr.c_str());
    printf("[🟐 ]Key:%s \n", k.GetBase16().c_str());
	  return true; // 仍然保持 return true;  表示 checkPrivKey 函数执行完成
  }

  return false; // 理论上这行代码不会被执行到，但为了完整性保留
}

void VanitySearch::checkAddr(int prefIdx, uint8_t *hash160, Int &key, int32_t incr, int endomorphism, bool mode) {

  vector<PREFIX_ITEM> *pi = nullptr;

  if (hasPattern) {
    // Wildcard search
    string addr = secp->GetAddress(searchType, mode, hash160); // 地址生成放在循环外部

    for (int i = 0; i < (int)inputPrefixes.size(); i++) {
      if (Wildcard::match(addr.c_str(), inputPrefixes[i].c_str(), caseSensitive)) {
        nbFoundKey++;  // 每次 checkPrivKey 调用都增加计数器 (Wildcard)
        updateFound(); // 更新 [Found] 显示 (Wildcard)
        checkPrivKey(inputPrefixes[i], key, incr, endomorphism, mode); // 调用 checkPrivKey，但不依赖返回值
      }
    }
    return;

  } else {
    // 非 Wildcard 搜索
    pi = prefixes[prefIdx].items;

    if (onlyFull) {
      // Full addresses
      for (int i = 0; i < (int)pi->size(); i++) {
        if (stopWhenFound && *((*pi)[i].found))
          continue;
        if (ripemd160_comp_hash((*pi)[i].hash160, hash160)) {
          *((*pi)[i].found) = true;
          nbFoundKey++;  // 每次 checkPrivKey 调用都增加计数器 (Full Address)
          updateFound(); // 更新 [Found] 显示 (Full Address)
          checkPrivKey((*pi)[i].prefix, key, incr, endomorphism, mode); // 调用 checkPrivKey，但不依赖返回值
        }
      }

    } else {
      string addr = secp->GetAddress(searchType, mode, hash160); // **移动到循环外部**
      // char a[64]; // 移除固定大小的 char 数组

      for (int i = 0; i < (int)pi->size(); i++) {
        if (stopWhenFound && *((*pi)[i].found))
          continue;

        string prefixToCheck = addr.substr(0, (*pi)[i].prefixLength); // 使用 substr

        if (prefixToCheck == (*pi)[i].prefix) {
          *((*pi)[i].found) = true;
          nbFoundKey++;  // 每次 checkPrivKey 调用都增加计数器 (Prefix Search)
          updateFound(); // 更新 [Found] 显示 (Prefix Search)
          checkPrivKey((*pi)[i].prefix, key, incr, endomorphism, mode); // 调用 checkPrivKey，但不依赖返回值
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
  //Point pte1[1];
  //Point pte2[1];

  // Point
  secp->GetHash160(searchType,compressed, p1, h0);
  prefix_t pr0 = *(prefix_t *)h0;
  if (hasPattern || prefixes[pr0].items)
    checkAddr(pr0, h0, key, i, 0, compressed);

  // Endomorphism #1
  //pte1[0].x.ModMulK1(&p1.x, &beta);
  //pte1[0].y.Set(&p1.y);

  //secp->GetHash160(searchType, compressed, pte1[0], h0);

  //pr0 = *(prefix_t *)h0;
  //if (hasPattern || prefixes[pr0].items)
    //checkAddr(pr0, h0, key, i, 1, compressed);

  // Endomorphism #2
  //pte2[0].x.ModMulK1(&p1.x, &beta2);
  //pte2[0].y.Set(&p1.y);

  //secp->GetHash160(searchType, compressed, pte2[0], h0);

  //pr0 = *(prefix_t *)h0;
  //if (hasPattern || prefixes[pr0].items)
    //checkAddr(pr0, h0, key, i, 2, compressed);

  // Curve symetrie
  // if (x,y) = k*G, then (x, -y) is -k*G
  //p1.y.ModNeg();
  //secp->GetHash160(searchType, compressed, p1, h0);
  //pr0 = *(prefix_t *)h0;
  //if (hasPattern || prefixes[pr0].items)
    //checkAddr(pr0, h0, key, -i, 0, compressed);

  // Endomorphism #1
  //pte1[0].y.ModNeg();

  //secp->GetHash160(searchType, compressed, pte1[0], h0);

  //pr0 = *(prefix_t *)h0;
  //if (hasPattern || prefixes[pr0].items)
    //checkAddr(pr0, h0, key, -i, 1, compressed);

  // Endomorphism #2
  //pte2[0].y.ModNeg();

  //secp->GetHash160(searchType, compressed, pte2[0], h0);

  //pr0 = *(prefix_t *)h0;
  //if (hasPattern || prefixes[pr0].items)
    //checkAddr(pr0, h0, key, -i, 2, compressed);

}

// ----------------------------------------------------------------------------
/*
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
*/


/*
// ----------------------------------------------------------------------------
void VanitySearch::getCPUStartingKey(int thId,Int& key,Point& startP) {

  if (rekey > 0) {
    key.Rand(256);
  } else {
    key.Set(&startKey);
    Int off((int64_t)thId);
    off.ShiftL(64);
    key.Add(&off);
  }
  Int km(&key);
  km.Add((uint64_t)CPU_GRP_SIZE / 2);
  startP = secp->ComputePublicKey(&km);
  if(startPubKeySpecified)
   startP = secp->AddDirect(startP,startPubKey);

}
*/
// ----------------------------------------------------------------------------
void VanitySearch::getCPUStartingKey(int thId, Int& key, Point& startP) {

  // !!! Random seed
  //rseed((unsigned long)time(NULL));// if not used OpenSSL
  //
  // Seed random number generator with performance counter
  //if (keys_seed_fl) { RandAddSeed(); }
  //
  Int one1;
  one1.SetInt32(1);
  key2.SetInt32(1);
  key3.SetInt32(1);
  key2.ShiftL((uint32_t)(Random_bits - 1));
  key3.ShiftL((uint32_t)Random_bits);
  key3.Sub(&one1);// for strcmp()
  //
  //printf("\nRandom Bit: %d keys range: %s:%s\n", Random_bits, key2.GetBase16().c_str(), key3.GetBase16().c_str());
  //while (1) {}//check
  //
  int Key_bits_length = 0;
  if (rekey > 0) {
    NewRandom:
	key.Rand(Random_bits);// bit 66
	//key2.SetBase16("20000000000000000");// min value bit 66
	//key3.SetBase16("3ffffffffffffffff");// max value bit 66
	bool keyOk = false;
	while ((!keyOk && strcmp(key.GetBase16().c_str(), key2.GetBase16().c_str()) < 0) || (!keyOk && strcmp(key.GetBase16().c_str(), key3.GetBase16().c_str()) > 0)) {//while (strcmp(key1.GetBase16().c_str(), key2.GetBase16().c_str()) < 0) {

		//printf("\nBit %d Base Key thId %d: %s < %s or > %s Rekey true \n", Random_bits, thId, key.GetBase16().c_str(), key2.GetBase16().c_str(), key3.GetBase16().c_str());
		Key_bits_length = key.GetBitLength();
		//printf("[🟑 ]Bit %d Base Key thId %d: %s < %s or > %s Rekey true \r", Key_bits_length, thId, key.GetBase16().c_str(), key2.GetBase16().c_str(), key3.GetBase16().c_str());
		//
		key.Rand(Random_bits);// bit 66
		//
		if (strcmp(key.GetBase16().c_str(), key2.GetBase16().c_str()) < 0 || strcmp(key.GetBase16().c_str(), key3.GetBase16().c_str()) > 0) {
			keyOk = false;
			//key.Rand(Random_bits);// bit 66
			//
		} else {
			keyOk = true;
			break;
		}
	}

	//printf("\nBit %d CPU Base Key thId %d: %s\n", Random_bits, thId, key.GetBase16().c_str());
	Key_bits_length = key.GetBitLength();
	if (Key_bits_length != Random_bits) goto NewRandom;// check
	//printf("[🟑 ]Bit %d CPU Base Key thId %d: %s\r", Key_bits_length, thId, key.GetBase16().c_str());
  } else {
    key.Set(&startKey);
    Int off((int64_t)thId);
    //off.ShiftL(64);
	//
	int nbBit = startKey.GetBitLength();
	off.ShiftL((uint32_t)(nbBit - 8));
	//
	key.Add(&off);
	//printf("[🟑 ]CPU Base Key thId %d: %s\r", thId, key.GetBase16().c_str());
  }
  Int km(&key);
  km.Add((uint64_t)CPU_GRP_SIZE / 2);
  startP = secp->ComputePublicKey(&km);
  if(startPubKeySpecified)
   startP = secp->AddDirect(startP,startPubKey);

}


void VanitySearch::FindKeyCPU(TH_PARAM *ph) {

  // Global init
  int thId = ph->threadId;
  counters[thId] = 0;

  // CPU Thread
  IntGroup *grp = new IntGroup(CPU_GRP_SIZE/2+1);

  // Group Init
  Int  key;
  Point startP;
  getCPUStartingKey(thId,key,startP);

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

  while (!endOfSearch) {

    if (ph->rekeyRequest) {
      getCPUStartingKey(thId, key, startP);
      ph->rekeyRequest = false;
    }

    // Fill group
    int i;
    int hLength = (CPU_GRP_SIZE / 2 - 1);

    for (i = 0; i < hLength; i++) {
      dx[i].ModSub(&Gn[i].x, &startP.x);
    }
    dx[i].ModSub(&Gn[i].x, &startP.x);  // For the first point
    dx[i+1].ModSub(&_2Gn.x, &startP.x); // For the next center point

    // Grouped ModInv
    grp->ModInv();

    // We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
    // We compute key in the positive and negative way from the center of the group

    // center point
    pts[CPU_GRP_SIZE/2] = startP;

    for (i = 0; i<hLength && !endOfSearch; i++) {

      pp = startP;
      pn = startP;

      // P = startP + i*G
      dy.ModSub(&Gn[i].y,&pp.y);

      _s.ModMulK1(&dy, &dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
      _p.ModSquareK1(&_s);            // _p = pow2(s)

      pp.x.ModNeg();
      pp.x.ModAdd(&_p);
      pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

      pp.y.ModSub(&Gn[i].x, &pp.x);
      pp.y.ModMulK1(&_s);
      pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);

      // P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
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

      pts[CPU_GRP_SIZE/2 + (i+1)] = pp;
      pts[CPU_GRP_SIZE/2 - (i+1)] = pn;

    }

    // First point (startP - (GRP_SZIE/2)*G)
    pn = startP;
    dyn.Set(&Gn[i].y);
    dyn.ModNeg();
    dyn.ModSub(&pn.y);

    _s.ModMulK1(&dyn, &dx[i]);
    _p.ModSquareK1(&_s);

    pn.x.ModNeg();
    pn.x.ModAdd(&_p);
    pn.x.ModSub(&Gn[i].x);

    pn.y.ModSub(&Gn[i].x, &pn.x);
    pn.y.ModMulK1(&_s);
    pn.y.ModAdd(&Gn[i].y);

    pts[0] = pn;

    // Next start point (startP + GRP_SIZE*G)
    pp = startP;
    dy.ModSub(&_2Gn.y, &pp.y);

    _s.ModMulK1(&dy, &dx[i+1]);
    _p.ModSquareK1(&_s);

    pp.x.ModNeg();
    pp.x.ModAdd(&_p);
    pp.x.ModSub(&_2Gn.x);

    pp.y.ModSub(&_2Gn.x, &pp.x);
    pp.y.ModMulK1(&_s);
    pp.y.ModSub(&_2Gn.y);
    startP = pp;

#if 0
    // Check
    {
      bool wrong = false;
      Point p0 = secp.ComputePublicKey(&key);
      for (int i = 0; i < CPU_GRP_SIZE; i++) {
        if (!p0.equals(pts[i])) {
          wrong = true;
          printf("[%d] wrong point\n",i);
        }
        p0 = secp.NextKey(p0);
      }
      if(wrong) exit(0);
    }
#endif

    // Check addresses
    if (0) {//if (useSSE) {

      for (int i = 0; i < CPU_GRP_SIZE && !endOfSearch; i += 4) {

        switch (searchMode) {
          case SEARCH_COMPRESSED:
            //checkAddressesSSE(true, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
            break;
          case SEARCH_UNCOMPRESSED:
            //checkAddressesSSE(false, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
            break;
          case SEARCH_BOTH:
            //checkAddressesSSE(true, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
            //checkAddressesSSE(false, key, i, pts[i], pts[i + 1], pts[i + 2], pts[i + 3]);
            break;
        }

      }

    } else {

      for (int i = 0; i < CPU_GRP_SIZE && !endOfSearch; i ++) {

        switch (searchMode) {
        case SEARCH_COMPRESSED:
          checkAddresses(true, key, i, pts[i]);
          break;
        case SEARCH_UNCOMPRESSED:
          checkAddresses(false, key, i, pts[i]);
          break;
        case SEARCH_BOTH:
          checkAddresses(true, key, i, pts[i]);
          checkAddresses(false, key, i, pts[i]);
          break;
        }

      }

    }

    key.Add((uint64_t)CPU_GRP_SIZE);
    //counters[thId]+= 6*CPU_GRP_SIZE; // Point + endo #1 + endo #2 + Symetric point + endo #1 + endo #2
	counters[thId]+= CPU_GRP_SIZE;

  }

  ph->isRunning = false;

}

/*
// ----------------------------------------------------------------------------

void VanitySearch::getGPUStartingKeys(int thId, int groupSize, int nbThread, Int *keys, Point *p) {

  for (int i = 0; i < nbThread; i++) {
    if (rekey > 0) {
      keys[i].Rand(256);
    } else {
      keys[i].Set(&startKey);
      Int offT((uint64_t)i);
      offT.ShiftL(80);
      Int offG((uint64_t)thId);
      offG.ShiftL(112);
      keys[i].Add(&offT);
      keys[i].Add(&offG);
    }
    Int k(keys + i);
    // Starting key is at the middle of the group
    k.Add((uint64_t)(groupSize / 2));
    p[i] = secp->ComputePublicKey(&k);
    if (startPubKeySpecified)
      p[i] = secp->AddDirect(p[i], startPubKey);
  }

}
*/

// ----------------------------------------------------------------------------

void VanitySearch::getGPUStartingKeys(int thId, int groupSize, int nbThread, Int *keys, Point *p) {

  // !!! Random seed
  //rseed((unsigned long)time(NULL));// if not used OpenSSL
  //
  // Seed random number generator with performance counter
  //if (keys_seed_fl) { RandAddSeed(); }
  //
  Int one1;
  one1.SetInt32(1);
  key2.SetInt32(1);
  key3.SetInt32(1);
  key2.ShiftL((uint32_t)(Random_bits - 1));
  key3.ShiftL((uint32_t)Random_bits);
  key3.Sub(&one1);// for strcmp()
  //
  for (int i = 0; i < nbThread; i++) {
    int Key_bits_length = 0;
	if (rekey > 0) {
      NewRandom:
	  keys[i].Rand(Random_bits);// BIT 66
	  //key2.SetBase16("20000000000000000");// min value bit 66
	  //key3.SetBase16("3ffffffffffffffff");// max value bit 66
	  //
	  bool keyOk = false;
	  while ((!keyOk && strcmp(keys[i].GetBase16().c_str(), key2.GetBase16().c_str()) < 0) || (!keyOk && strcmp(keys[i].GetBase16().c_str(), key3.GetBase16().c_str()) > 0)) {//while (strcmp(key1.GetBase16().c_str(), key2.GetBase16().c_str()) < 0) {
		  // print check
		  //printf("GPU Base Key: %s < %s OR Key: %s > %s Rekey true \n", keys[i].GetBase16().c_str(), key2.GetBase16().c_str(), keys[i].GetBase16().c_str(), key3.GetBase16().c_str());
		  //
		  keys[i].Rand(Random_bits);// BIT 66
		  //
		  if (strcmp(keys[i].GetBase16().c_str(), key2.GetBase16().c_str()) < 0 || strcmp(keys[i].GetBase16().c_str(), key3.GetBase16().c_str()) > 0) {
			keyOk = false;
			//keys[i].Rand(Random_bits);// BIT 66
			//
		  } else {
			keyOk = true;
			break;
		  }
	  }
	  // print 20 keys
	  //if (i < 10 || i > nbThread - 10) { printf("Bit %d GPU Base Key %d: %s\n", Random_bits, i, keys[i].GetBase16().c_str()); }
	  Key_bits_length = keys[i].GetBitLength();
	  if (Key_bits_length != Random_bits) goto NewRandom;// check
	  //if (i < 10 || i > nbThread - 10) { printf("[🟑 ]Bit %d GPU Base Key %d: %s\r", Key_bits_length, i, keys[i].GetBase16().c_str()); }
	  //
    } else {
      //
	  keys[i].Set(&startKey);
      Int offT((uint64_t)i);
      //offT.ShiftL(32);
      Int offG((uint64_t)thId);
      //offG.ShiftL(40);
	  // new offset
	  int nbBit = startKey.GetBitLength();
	  offT.ShiftL((uint32_t)(nbBit / 2));
	  offG.ShiftL((uint32_t)(nbBit - 4));
	  //
      keys[i].Add(&offT);
      keys[i].Add(&offG);
	  //if (i < 10 || i > nbThread - 10) { printf("[🟑 ]Bit %d GPU startKey Base Key %d: %s\r", Random_bits, i, keys[i].GetBase16().c_str()); }
	  //
    }
    //Int k(keys + i);
	Int k(keys[i]);
    // Starting key is at the middle of the group
    k.Add((uint64_t)(groupSize / 2));
    p[i] = secp->ComputePublicKey(&k);
    if (startPubKeySpecified)
      p[i] = secp->AddDirect(p[i], startPubKey);
  }

}

// ----------------------------------------------------------------------------

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

  printf("[🟑 ]GPU: %s\n",g.deviceName.c_str());

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
      //counters[thId] += 6ULL * STEP_SIZE * nbThread; // Point +  endo1 + endo2 + symetrics
	  counters[thId] += STEP_SIZE * nbThread;
    }

  }

  delete[] keys;
  delete[] p;

#else
  ph->hasStarted = true;
  printf("GPU code not compiled, use -DWITHGPU when compiling.\r");
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

  printf("[🟑 ]Number of CPU thread: %d\n", nbCPUThread);

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
    uint64_t count = getCPUCount() + gpuCount;

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
      printf("[🟑 ][Ts %.2f Mkey/s][GPU %.2f Mkey/s][Total 2^%.2f]%s[Found %d]\r",
        avgKeyRate / 1000000.0, avgGpuKeyRate / 1000000.0,
          log2((double)count), GetExpectedTime(avgKeyRate, (double)count).c_str(),nbFoundKey);
    }

    if (rekey > 0) {
      if ((count - lastRekey) > (1000000 * rekey)) {
        // Rekey request
        rekeyRequest(params);
        lastRekey = count;
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
