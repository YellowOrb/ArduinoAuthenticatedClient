// An example of a REST Client using HMAC for authentication
//
// Change secretKey to match your secret key and also change the id to match yours. The id is the number before
// the HMAC signature in authHeaderStr, the 3 before the : in the code below.
//
//
// Sketch uses 24,818 bytes (80%) of program storage space. Maximum is 30,720 bytes.
// Global variables use 1,072 bytes (52%) of dynamic memory, leaving 976 bytes for local variables. Maximum is 2,048 bytes.

#include <SoftwareSerial.h>
#include <SIM900.h>
#include <RestClient.h>
#include <sha1.h>
#include <MD5.h>
#include <Base64.h>

SIM900GPRS gsm;
SIM900Client client(&gsm);
RestClient rest(&client, "www.yelloworb.com");

String response;
int responseCode;
const char PROGMEM secretKey[] = "wJ+LuCJKuFOcEdV0rdbp0BtkLdLm/EvH31KwiILc/xC35zgDd+KMBTuvmpH9uhNtOvfTr8rl7j6CHVSM8BB7XA==";
char b64[29];
uint8_t tries;

//char *dateStr = "Date: DAY, DD MON YEAR HH:MM:SS GMT";
char *dateStr ="Date: Thu, 13 Nov 2014 14:18:11 GMT";
#define DATE_STR_CONTENT_START 6
char *contentMD5Str = "Content-Md5: 1234567890123456789012=="; //22 characters plus 2 = padding; http://stackoverflow.com/a/13296298
#define CONTENT_MD5_STR_CONTENT_START 13
char *contentTypeHeaderStr = "Content-Type: application/x-www-form-urlencoded";
#define CONTENT_TYPE_STR_CONTENT_START 14
char *authHeaderStr = "Authorization: APIAuth 3:123456789012345678901234567=";
#define AUTH_HEADER_STR_CONTENT_START 25                                           

char *uriPathStr="/measures.json";
char *bodyStr = "measure[temperature]=12";

/**
 * creates a HMAC signature based on the secret key and the four strings. The secret should be stored in PROGMEM or this will not work
 */
uint8_t *getHMACSignature_P(const uint8_t *secret, uint8_t secretSize, char *string1, char *string2, char * string3, char *string4) {
  Sha1.initHmac_P(secret, secretSize);
  Sha1.print(string1);Sha1.print(',');
  Sha1.print(string2);Sha1.print(',');
  Sha1.print(string3);Sha1.print(',');
  Sha1.print(string4);  
  return Sha1.resultHmac();
}

bool initializeRESTClient() {
  if(!gsm.turnOn()) {
    return false;
  }
  
  Serial.println(F("Module on"));
  
  tries=0;
  while(gsm.begin() != GSM_READY) {
    Serial.println(F("gsm setup problem"));
    tries++;
    if(tries >= 5) {
      return false;
    }
    delay(1000);
  }
  
  tries=0;
  while(GPRS_READY != gsm.attachGPRS("online.telia.se", NULL, NULL)) {
    Serial.println(F("gprs join network error"));
    tries++;
    if(tries >= 5) {
      return false;
    }
    delay(1000);
  }
  return true;
}

void endRESTClient() {
  gsm.detachGPRS(); 
  gsm.shutdown();
}

bool postValue(char *resourcePath, char *body) {
  if(false == client.connect("www.yelloworb.com", 2000)) {
      Serial.println(F("connect error"));
      return false;
  }

  rest.setHeader("Accept: application/json");
  
  // write time into dateStr
  rest.setHeader(dateStr);
  
  unsigned char* hash=MD5::make_hash(body);  
  base64_encode(b64, (char *)hash, 16);
  strncpy(&contentMD5Str[CONTENT_MD5_STR_CONTENT_START], b64, 22);
  free(hash);
  rest.setHeader(contentMD5Str);
    
  base64_encode(b64,
    (char *)getHMACSignature_P((uint8_t *)secretKey, sizeof(secretKey)-1,
      &contentTypeHeaderStr[CONTENT_TYPE_STR_CONTENT_START],
      &contentMD5Str[CONTENT_MD5_STR_CONTENT_START],
      resourcePath,
      &dateStr[DATE_STR_CONTENT_START]),
    20);

  strncpy(&authHeaderStr[AUTH_HEADER_STR_CONTENT_START], b64, 27);
  rest.setHeader(authHeaderStr);

  client.beginWrite();
  rest.post(resourcePath, body);
  client.endWrite();

  responseCode = rest.readResponse(&response);
  Serial.print(F("Response: "));
  Serial.println(responseCode);
  Serial.println(response);
    
  client.stop();
  
  if(responseCode==200  || responseCode==201) {
    return true;
  } else {
    return false;
  }
}

void setup(){
  Serial.begin(9600);
  Serial.println(F("HMAC REST Client started"));
  
  if(initializeRESTClient()) {
    delay(1000);
 
    Serial.println(F("Send request"));
    if(postValue("/measures.json", "measure[temperature]=12")){
      Serial.println(F("New value posted!"));
    }else{
      Serial.println(F("Failed to post value"));
    }
  } else {
    Serial.println(F("Failed setup GPRS"));
  }
  endRESTClient();
  Serial.println(F("Done!"));
}

void loop(){
}
