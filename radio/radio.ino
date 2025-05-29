//************************************
//*    audioI2S-- I2S audiodecoder for ESP32,                                                              *
//************************************
//
// first release on 11/2018
// Version 3  , Jul.02/2020
//
//
// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
// FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR
// OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
//

#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"

#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25

Audio audio;
WiFiMulti wifiMulti;
String ssid =     "NORA 24";
String password = "eloelo520";


void setup() {

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid.c_str(), password.c_str());
  wifiMulti.run();
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    wifiMulti.run();
  }
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(12); // 0...21

  audio.connecttohost("http://ml01.cdn.eurozet.pl/mel-krk.mp3"); //  eska2 online
}

void loop()
{
  audio.loop();

}