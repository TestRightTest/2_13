#include <HTTPClient.h>
#include <HTTPUpdate.h>


//String serverName = "http://43.204.189.245/MBScan-Dashboard/dashboard/update-sensor.php";
//String serverIP = "43.204.189.245";
//String serverName = "http://43.204.189.245/uat/mbscan/iot/";

String serverIP = "mbscan.testright.in";
String serverName = "http://mbscan.testright.in/mbscan/iot/";
String startLog = "startLog";
String endLog = "endLog";
String oldLog = "oldLog";
String checkOta = "check_ota";
String fetchOta = "/mbscan/iot/fetch_ota";
String fetchParameters = "fetch_config";

channelData old_channelData;
String old_fileName;
String old_testCountID;
int startPos = 0;

 
