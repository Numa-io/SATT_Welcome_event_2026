//1年生向け電装講習会用
//BMP280 SPIプログラム
//1年生向け電装講習会用
//BMP280 SPIプログラム
#include <SPI.h>

//アドレス指定
#define BMP280_ADDR 0x76  //BMP280デバイスアドレス
//################################################################################
#define CONFIG 0xF5          //コンフィグレジスタ(待機時間，フィルタ，3/4線式SPIの切り替え)
#define t_sb 0b00100000      //待機時間設定bit -> 62.5 [ms]
#define filter 0b00000000    //フィルタ設定bit -> フィルタなし
#define spi3w_en 0b00000000  //SPI形式指定bit -> 4線式
//################################################################################
#define CTRL_MEAS 0xF4          //気圧・温度測定に関する設定を行うレジスタ
#define osrs_t 0b00100000       //温度測定のオーバーサンプリング設定bit -> oversampling x1
#define osrs_p 0b00000100       //気圧測定のオーバーサンプリング設定bit -> oversampling x1
#define sleep_mode 0b00000000   //モード設定bit -> sleep mode -> スリープ
#define forced_mode 0b00000001  //モード設定bit -> forced mode -> 単発測定
#define normal_mode 0b00000111  //モード設定bit -> normal mode -> 連続測定
//################################################################################
#define CALIB_00 0x88   //補正データdig_T1~dig_P9のデータが保存されている領域の先頭レジスタ
#define Press_REG 0xF7  //気圧の測定結果が格納されているレジスタ

//SPI設定 (クロック周波数: 5MHz, ビット順: MSBFIRST, モード: SPI_MODE0)
SPISettings settings(5000000, MSBFIRST, SPI_MODE0);

//温度補正データを格納する変数(16bitが3個->8bitが6個)
uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;

//気圧補正データを格納する変数(16bitが9個->8bitが18個)
uint16_t dig_P1;
int16_t dig_P2;
int16_t dig_P3;
int16_t dig_P4;
int16_t dig_P5;
int16_t dig_P6;
int16_t dig_P7;
int16_t dig_P8;
int16_t dig_P9;

//補正データを格納する配列
unsigned char dac[24];  //8bitが温度用「6個」と気圧用「18個」で計「24個」
unsigned int i;

int32_t t_fine;
int32_t adc_P, adc_T;


//################################################################################
void setup() {
  //シリアル通信初期化
  Serial.begin(9600);  //シリアル通信を9600bpsで初期化
  while (!Serial) {}   //シリアルのセッティング完了するまで待機

  pinMode(SS, OUTPUT);     //SSは予約語でpin[10]を意味する
  digitalWrite(SS, HIGH);  // 初期状態はHIGH

  //SPIを初期化
  SPI.begin();

  //BMP280動作設定->CONFIGレジスタに書き込む
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);        //通信開始の合図
  SPI.transfer(CONFIG & 0x7F);  //ライト命令
  SPI.transfer(t_sb | filter | spi3w_en);
  digitalWrite(SS, HIGH);  //通信終了の合図
  SPI.endTransaction();

  //BMP280測定条件設定->CTRL_MEASレジスタに書き込む
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);           //通信開始の合図
  SPI.transfer(CTRL_MEAS & 0x7F);  //ライト命令
  SPI.transfer(osrs_t | osrs_p | normal_mode);
  digitalWrite(SS, HIGH);  //通信終了の合図
  SPI.endTransaction();

  //BMP280補正データ(温度と気圧)の取得
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);  //通信開始の合図

  //リード命令
  SPI.transfer(CALIB_00 | 0x80);

  //バーストリード(補正データ : 24 byte)
  for (i = 0; i < 24; i++) {
    dac[i] = SPI.transfer(0x00);
  }

  digitalWrite(SS, HIGH);  //通信終了の合図
  SPI.endTransaction();

  //or演算で8bitデータを合成して16bitデータに直す
  dig_T1 = ((uint16_t)((dac[1] << 8) | dac[0]));
  dig_T2 = ((int16_t)((dac[3] << 8) | dac[2]));
  dig_T3 = ((int16_t)((dac[5] << 8) | dac[4]));

  dig_P1 = ((uint16_t)((dac[7] << 8) | dac[6]));
  dig_P2 = ((int16_t)((dac[9] << 8) | dac[8]));
  dig_P3 = ((int16_t)((dac[11] << 8) | dac[10]));
  dig_P4 = ((int16_t)((dac[13] << 8) | dac[12]));
  dig_P5 = ((int16_t)((dac[15] << 8) | dac[14]));
  dig_P6 = ((int16_t)((dac[17] << 8) | dac[16]));
  dig_P7 = ((int16_t)((dac[19] << 8) | dac[18]));
  dig_P8 = ((int16_t)((dac[21] << 8) | dac[20]));
  dig_P9 = ((int16_t)((dac[23] << 8) | dac[22]));  // 配列要素数を23/22に正しく修正

  delay(1000);  //1秒待機
}


//################################################################################
void loop() {
  int32_t temp_cal;
  uint32_t pres_cal;
  float temp, pres;

  //測定データ取得
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);  //通信開始の合図

  //リード命令
  SPI.transfer(Press_REG | 0x80);

  //バーストリード(測定データ : 6 byte)
  for (i = 0; i < 6; i++) {
    dac[i] = SPI.transfer(0x00);
  }

  digitalWrite(SS, HIGH);  //通信終了の合図
  SPI.endTransaction();

  //or演算でデータを合成
  adc_P = ((uint32_t)dac[0] << 12) | ((uint32_t)dac[1] << 4) | ((dac[2] >> 4) & 0x0F);
  adc_T = ((uint32_t)dac[3] << 12) | ((uint32_t)dac[4] << 4) | ((dac[5] >> 4) & 0x0F);

  pres_cal = BMP280_compensate_P_int32(adc_P);
  temp_cal = BMP280_compensate_T_int32(adc_T);

  pres = (float)pres_cal / 100.0;
  temp = (float)temp_cal / 100.0;

  //シリアルモニタ送信
  Serial.print("Pressure:");
  Serial.print(pres, 2);
  Serial.print("hPa ");
  Serial.print("Temp:");
  Serial.print(temp, 2);
  Serial.println("°C ");

  delay(1000);  //1秒待機
}


//################################################################################
//温度補正 関数
int32_t BMP280_compensate_T_int32(int32_t adc_T) {
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}


//################################################################################
//気圧補正 関数
uint32_t BMP280_compensate_P_int32(int32_t adc_P) {
  int32_t var1, var2;
  uint32_t p;
  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
  var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
  var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);
  if (var1 == 0) {
    return 0;  // avoid exception caused by division by zero
  }
  p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
  if (p < 0x80000000) {
    p = (p << 1) / ((uint32_t)var1);
  } else {
    p = (p / (uint32_t)var1) * 2;
  }
  var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
  var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
  return p;
}