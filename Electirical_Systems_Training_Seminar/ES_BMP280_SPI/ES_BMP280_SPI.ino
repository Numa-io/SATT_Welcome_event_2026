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
#define normal_mode 0b00000011  //モード設定bit -> normal mode -> 連続測定
//################################################################################
#define CALIB_00 0x88  //補正データdig_T1~dig_P9のデータが保存されている領域の先頭レジスタ
#define Press_REG 0xF7  //気圧の測定結果が格納されているレジスタ

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

  pinMode(SS, OUTPUT);  //SSは予約語でpin[10]を意味する

  //SPIを初期化, SCK[13], MISO[12], MOSI[11], SS[10]の各ピンのモードはOUTPUT
  SPI.begin();  //I2Cを初期化

  //SPIのModeを「MODE0」に設定 -> CPHA(クロック位相) : 0, CPOL(クロック極性) : 0
  //BMP280はMODE0([00])かMODE3([11])に対応
  //アイドル状態(通信していないとき：SCKはLOW, SSはHIGH)
  //クロックの立ち上がりでデータを出力
  SPI.setDataMode(SPI_MODE0);

  //SPI送受信用のビットオーダーを「MSBFIRST」に設定
  //データシートのタイミングチャートに，最上位bitが時間的に先であることが明記
  SPI.setBitOrder(MSBFIRST);

  //BMP280動作設定->CONFIGレジスタに書き込む
  digitalWrite(SS, LOW);  //通信開始の合図 -> SSピンの出力をLOW(0V)に設定 (アクティブLow)
  SPI.transfer(CONFIG & 0x7F);  //動作設定 -> bit[7]に[W/R]bit('w'), bit[6]~[0]にレジスタアドレスの下位7bit 送受信が同時に行われるためSPI.transferでいい
  SPI.transfer(t_sb | filter | spi3w_en); //「単発測定」,「フィルタなし」,「SPI 4線式」
  digitalWrite(SS, HIGH);	//通信終了の合図 -> SSピンの出力をHIGH(5V)に設定

  //BMP280測定条件設定->CTRL_MEASレジスタに書き込む
  digitalWrite(SS, LOW);  //通信開始の合図 -> SSピンの出力をLOW(0V)に設定 (アクティブLow)
  SPI.transfer(CTRL_MEAS & 0x7F);	//測定条件設定 ->  -> bit[7]に[W/R]bit('w'), bit[6]~[0]にレジスタアドレスの下位7bit
  SPI.transfer(osrs_t | osrs_p | normal_mode);	//「温度・気圧オーバーサンプリングx1」,「連続測定モード」
  digitalWrite(SS, HIGH);	//通信終了の合図 -> SSピンの出力をHIGH(5V)に設定

  //BMP280補正データ(温度と気圧)の取得
  digitalWrite(SS, LOW);  //通信開始の合図 -> SSピンの出力をLOW(0V)に設定 (アクティブLow)
 
  //出力データバイトを「補正データ」の先頭アドレスに指定，読み出しフラグを立てる
  SPI.transfer(CALIB_00 | 0x80); //先頭レジスタから読み出す-> bit[7]に[W/R]bit('R'), bit[6]~[0]にレジスタアドレスの下位7bit 0x80：ビットマスク

  //バーストリード(補正データ : 24 byte) ２４回データを受け取っている
  for (i=0; i < 24; i++){
    dac[i] = SPI.transfer(0x00);	//dacにSPIデバイス「BME280」のデータ読み込み -> 同時にダミーデータ(0x00)送信∵送受信を同時にしなければいけないため適当に送るデータを決めなきゃいけない dac[]がバッファの役割
  }

  digitalWrite(SS, HIGH);	//通信終了の合図 -> SSピンの出力をHIGH(5V)に設定

  //or演算で8bitデータを合成して16bitデータに直す I2Cと同じことをしている
  dig_T1 = ((uint16_t)((dac[1] << 8) | dac[0])); //(uint16_t)は((dac[1] << 8) | dac[0])という変数を8bitから16bitに拡張している（この操作をキャストという）
  dig_T2 = ((int16_t)((dac[3] << 8) | dac[2])); //int a=8 (float)(a)とするとaをfloatにできる
  dig_T3 = ((int16_t)((dac[5] << 8) | dac[4])); //シフト演算を使いたくなければ2^8かければいい（8こ左シフトしているから2^8）

  dig_P1 = ((uint16_t)((dac[7] << 8) | dac[6])); //環境により型の扱い方(ex:intは環境により4byteで扱われたり8byteで扱われたりする)から明記している
  dig_P2 = ((int16_t)((dac[9] << 8) | dac[8]));
  dig_P3 = ((int16_t)((dac[11] << 8) | dac[10]));
  dig_P4 = ((int16_t)((dac[13] << 8) | dac[12]));
  dig_P5 = ((int16_t)((dac[15] << 8) | dac[14]));
  dig_P6 = ((int16_t)((dac[17] << 8) | dac[16]));
  dig_P7 = ((int16_t)((dac[19] << 8) | dac[18]));
  dig_P8 = ((int16_t)((dac[21] << 8) | dac[20]));
  dig_P9 = ((int16_t)((dac[23] << 8) | dac[22]));

  delay(1000);  //1000msec待機(1秒待機)
}


//################################################################################
void loop() {
  int32_t temp_cal;
  uint32_t pres_cal;
  float temp, pres;

  //測定データ取得
  digitalWrite(SS, LOW);  //通信開始の合図 -> SSピンの出力をLOW(0V)に設定 (アクティブLow)

  //出力データバイトを「補正データ」の先頭アドレスに指定，読み出しフラグを立てる
  SPI.transfer(Press_REG | 0x80); //先頭レジスタから読み出す-> bit[7]に[W/R]bit('R'), bit[6]~[0]にレジスタアドレスの下位7bit

  //バーストリード(測定データ : 6 byte)
  for (i=0; i < 6; i++){
    dac[i] = SPI.transfer(0x00);	//dacにSPIデバイス「BME280」のデータ読み込み -> 同時にダミーデータ(0x00)送信
  }

  digitalWrite(SS, HIGH);	//通信終了の合図 -> SSピンの出力をHIGH(5V)に設定
  
  //or演算でデータを合成 
  //(有意なのは20bitでデータは上位から詰めるように8bitごとに3分割されている->最下位データで有効なのは上位4bitだけ)
  adc_P = ((uint32_t)dac[0] << 12) | ((uint32_t)dac[1] << 4) | ((dac[2] >> 4) & 0x0F);
  adc_T = ((uint32_t)dac[3] << 12) | ((uint32_t)dac[4] << 4) | ((dac[5] >> 4) & 0x0F);

  pres_cal = BMP280_compensate_P_int32(adc_P);  //気圧データ補正計算 今回はデータシートに記載されている 書かれてないときはキャリブレーションの仕方も考えなければいけない
  temp_cal = BMP280_compensate_T_int32(adc_T);  //温度データ補正計算 今回はデータシートに記載されている 書かれてないときはキャリブレーションの仕方も考えなければいけない

  pres = (float)pres_cal / 100.0;  //気圧データを実際の値に計算
  temp = (float)temp_cal / 100.0;  //温度データを実際の値に計算

  //シリアルモニタ送信
  Serial.print("Pressure:");  //文字列「Pressure:」をシリアルモニタに送信
  Serial.print(pres, 2);      //「pres」をシリアルモニタに送信
  Serial.print("hPa ");       //文字列「hPa 」をシリアルモニタに送信
  Serial.print("Temp:");      //文字列「Temp:」をシリアルモニタに送信
  Serial.print(temp, 2);      //「temp」をシリアルモニタに送信
  Serial.println("°C ");      //文字列「°C 」をシリアルモニタに送信

  delay(1000);  //1000msec待機(1秒待機)
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