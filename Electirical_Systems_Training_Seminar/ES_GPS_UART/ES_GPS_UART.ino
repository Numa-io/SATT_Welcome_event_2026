// Arduino UNO R4用: ライブラリ非依存 GPS（NMEA）解析・生データ表示プログラム

#define GPS_SERIAL Serial1  // UNO R4のピン0(RX)/ピン1(TX)を使用

char buffer[128];
int bufIndex = 0;

// 度分表記 (ddmm.mmmm / dddmm.mmmm) を 10進数度 (dd.dddddd) に変換する関数
double convertNMEAToDegrees(double nmeaVal) {
  int degrees = (int)(nmeaVal / 100.0);
  double minutes = nmeaVal - (degrees * 100.0);
  return degrees + (minutes / 60.0);
}

// NMEAセンテンスの解析関数
void parseNMEA(char* line) {
  // $GPRMC または $GNRMC センテンスのみを対象とする
  if (strncmp(line, "$GPRMC", 6) != 0 && strncmp(line, "$GNRMC", 6) != 0) {
    return;
  }

  // カンマ区切りで各フィールドの開始ポインタを取得
  char* fields[15];
  int fieldCount = 0;
  
  char* ptr = line;
  fields[fieldCount++] = ptr;
  
  while (*ptr != '\0') {
    if (*ptr == ',') {
      *ptr = '\0'; // カンマをNULL終端に置換して文字列を分割
      fields[fieldCount++] = ptr + 1;
      if (fieldCount >= 15) break;
    }
    ptr++;
  }

  // フィールド数が不足している場合は処理中断
  if (fieldCount < 7) return;

  // Field 2: 測位ステータス (A = 有効, V = 未測位/警告)
  char status = fields[2][0];
  if (status != 'A') {
    Serial.println("  └─ [解析結果] 位置情報: 測位中（衛星検索中...）");
    return;
  }

  // 各フィールドの抽出
  double rawLat = atof(fields[3]);
  char ns = fields[4][0];
  double rawLon = atof(fields[5]);
  char ew = fields[6][0];

  // 10進数度へ変換
  double lat = convertNMEAToDegrees(rawLat);
  if (ns == 'S') lat = -lat; // 南緯の場合は負の値

  double lon = convertNMEAToDegrees(rawLon);
  if (ew == 'W') lon = -lon; // 西経の場合は負の値

  // 結果出力（視認性のためインデントを追加）
  Serial.print("  └─ [解析結果] 緯度: ");
  Serial.print(lat, 6);
  Serial.print(" | 経度: ");
  Serial.println(lon, 6);
}

void setup() {
  Serial.begin(115200);               // PC出力用シリアル通信
  while (!Serial && millis() < 3000);  // シリアルモニタ起動待機
  
  GPS_SERIAL.begin(9600);             // NEO-6M用ボーレート
  Serial.println("GPSデータ解析プログラム開始（生データ表示モード）");
}

void loop() {
  // GPSから1文字ずつ受信
  while (GPS_SERIAL.available() > 0) {
    char c = GPS_SERIAL.read();
    
    // 改行文字（センテンス終端）を検出した場合
    if (c == '\n' || c == '\r') {
      if (bufIndex > 0) {
        buffer[bufIndex] = '\0'; // 文字列終端処理
        
        // 1. 生データ（NMEAセンテンス）を出力
        Serial.print("[RAW] ");
        Serial.println(buffer);
        
        // 2. 解析処理を実行（$GPRMC / $GNRMC の場合のみ下に解析結果が表示される）
        parseNMEA(buffer);
        
        bufIndex = 0;             // バッファリセット
      }
    } else {
      // バッファに文字を追加（オーバーフロー防止チェック付）
      if (bufIndex < (int)(sizeof(buffer) - 1)) {
        buffer[bufIndex++] = c;
      }
    }
  }
}