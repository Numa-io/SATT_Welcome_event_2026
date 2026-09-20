//1年生向け電装講習会用
//BMP280 I2Cプログラム
#include <Wire.h>

//アドレス指定
#define BMP280_ADDR 0x76  //BMP280デバイスアドレス //#defineは前の文字列を次の文字列と解釈（前の文字を入れると後ろの文字が出てくる）プログラムを見たときにわかりやすくするためにdefineを使う
//################################################################################
#define CONFIG 0xF5	//コンフィグレジスタ(待機時間，フィルタ，3/4線式SPIの切り替え)
#define t_sb 0b00100000 //待機時間設定bit -> 62.5 [ms]
#define filter 0b00000000 //フィルタ設定bit -> フィルタなし
#define spi3w_en 0b00000000 //SPI形式指定bit -> 4線式
//################################################################################
#define CTRL_MEAS 0xF4	//気圧・温度測定に関する設定を行うレジスタ
#define osrs_t 0b00100000 //温度測定のオーバーサンプリング設定bit -> oversampling x1
#define osrs_p 0b00000100 //気圧測定のオーバーサンプリング設定bit -> oversampling x1
#define sleep_mode 0b00000000 //モード設定bit -> sleep mode -> スリープ
#define forced_mode 0b00000001  //モード設定bit -> forced mode -> 単発測定
#define normal_mode 0b00000011  //モード設定bit -> normal mode -> 連続測定
//################################################################################
#define Press_REG 0xF7  //気圧の測定結果が格納されているレジスタ 

//温度補正データを格納する変数(16bitが3個->8bitが6個)
uint16_t dig_T1; //※uintはunsigned intで符号なし
int16_t  dig_T2; //int16_tはint型で16bit
int16_t  dig_T3; //int20_tとは中途半端な数字はない(8,16,32とかはあるはず)

//気圧補正データを格納する変数(16bitが9個->8bitが18個)
uint16_t dig_P1;
int16_t  dig_P2;
int16_t  dig_P3;
int16_t  dig_P4;
int16_t  dig_P5;
int16_t  dig_P6;
int16_t  dig_P7;
int16_t  dig_P8;
int16_t  dig_P9;

//補正データを格納する配列
unsigned char dac[24];	//8bitが温度用「6個」と気圧用「18個」で計「24個」
unsigned int i;

int32_t t_fine;
int32_t adc_P, adc_T;


//################################################################################
void setup() {
   //シリアル通信初期化
   Serial.begin(9600);//シリアル通信を9600bpsで初期化
   while (!Serial) {}  //シリアルのセッティング完了するまで待機

   //I2C初期化
   Wire.begin();//I2Cを初期化。I2Cを使うときはA4,A5ピン？は使えない（∵I2Cの使うピンと共通だから）→この文はA4A5ピンをI2Cの回路として使うとして設定している
   //Wire.begin()のカッコ内に文字入れるとslaveとして使える。masterとして使うときは何も書かない
   //BMP280動作設定->CONFIGレジスタに書き込む
   Wire.beginTransmission(BMP280_ADDR);//I2Cスレーブ「BMP280」へのデータ送信開始。Wire.hでの書き方はこれ。0x76をバイナリで111|0110|R/Wの1ビットを加えて送る。8bitでデバイスアドレス書かれてたらR/Wの1bit削る
   Wire.write(CONFIG);//動作設定。レジスタアドレスを送っている。CONFIGでいいのはdefineしてあるから。動作の設定ができるレジスタ。どうすればいいかはデータシートへ
   Wire.write(t_sb | filter | spi3w_en);//「単発測定」、「フィルタなし」、「SPI 4線式」 |はor演算→t_sb、filter、spi3w_enの３つでor演算が適用される※可読性のためt_sb、filter、spi3w_enはdefineで定義されてる。ベタ打ちしても大丈夫
   Wire.endTransmission();//I2Cスレーブ「BMP280」のデータ送信終了 stop conditionが送られている ここでいったん通信が切れる

   //BMP280測定条件設定->CTRL_MEASレジスタに書き込む 基本的に54-57行目と同じようなことをしている。変わっているのは通信しているレジスタ
   Wire.beginTransmission(BMP280_ADDR);//I2Cスレーブ「BMP280」へのデータ送信開始
   Wire.write(CTRL_MEAS);//測定条件設定
   Wire.write(osrs_t | osrs_p | normal_mode);//「温度・気圧オーバーサンプリングx1」、「連続測定モード」 modeは3つdefineされているが今回使うのはnormal_mode
   Wire.endTransmission();//I2Cスレーブ「BMP280」へのデータ送信終了 ここまででBMP280の測定のための準部は完了

   //BMP280補正データ(温度と気圧)の取得
   Wire.beginTransmission(BMP280_ADDR);//I2Cスレーブ「BMP280」へのデータ送信開始
   Wire.write(0x88);//データの要求先を「補正データ」のレジスタに指定 どこからのデータが欲しいかを0x88というレジスタアドレスで指定
   Wire.endTransmission();//I2Cスレーブ「BMP280」へのデータ送信終了
  
   Wire.requestFrom(BMP280_ADDR, 24);//I2Cデバイス「BMP280」に24Byteのデータ要求(一括読み込み)読み込みモード 24byteほしいということだから24という数字を送っている 1つ2byte×12個 バッファに24byte分のデータがたまるように指示している
   for (i=0; i<24; i++){ //バッファは集荷センター的役割。arduinoに用意されている (このバッファはfast in fast out方式)
     while (Wire.available() == 0 ){}//バッファに要求したデータがたまるまで待機 (Wire.availableはバッファの中に何バイトあるか教えてくれる関数?)
     dac[i] = Wire.read();//dacにI2Cデバイス「BMP280」のデータを格納(バッファ->配列dac) Wire.readは1byteずつ読み込む
   }
  
   //or演算で8bitデータを合成して16bitデータに直す 8bitずつデータが送られているからそれをを16bitにしている <<は(2^8?)をかけているのと一緒つまり8bit桁を左にずらしている シフト演算という
   dig_T1 = ((uint16_t)((dac[1] << 8) | dac[0])); //桁をずらしたうえでor演算 もともとデータがあった部分はゼロで埋まるdacという配列の1番目と0番目をくっつけた
   dig_T2 = ((int16_t)((dac[3] << 8) | dac[2])); //目的は16bitにしたい
   dig_T3 = ((int16_t)((dac[5] << 8) | dac[4])); //(int16_tはもともと8bitだから16bitに拡張している)

   dig_P1 = ((uint16_t)((dac[7] << 8) | dac[6])); //下位bitが先に入っているからシフトしてor演算をするのはこの順番になっている（これはデータシートに書いてある）(ものによっては先に上位bitが先に入っていることもある)
   dig_P2 = ((int16_t)((dac[9] << 8) | dac[8])); //データの結合はよくやるので方法は覚えておく
   dig_P3 = ((int16_t)((dac[11] << 8) | dac[10]));
   dig_P4 = ((int16_t)((dac[13] << 8) | dac[12]));
   dig_P5 = ((int16_t)((dac[15] << 8) | dac[14]));
   dig_P6 = ((int16_t)((dac[17] << 8) | dac[16]));
   dig_P7 = ((int16_t)((dac[19] << 8) | dac[18]));
   dig_P8 = ((int16_t)((dac[21] << 8) | dac[20]));
   dig_P9 = ((int16_t)((dac[23] << 8) | dac[22]));

   delay(1000);//1000msec待機(1秒待機)
 }


//################################################################################
void loop() {
  int32_t  temp_cal;
  uint32_t pres_cal;
  float temp, pres;
  
  //測定データ取得
  Wire.beginTransmission(BMP280_ADDR);//I2Cスレーブ「BMP280」へのデータ送信開始
  Wire.write(Press_REG);//データの要求先を「気圧データ」のレジスタに指定
  Wire.endTransmission();//I2Cスレーブ「BMP280」へのデータ送信終了
 
  Wire.requestFrom(BMP280_ADDR, 6);//I2Cデバイス「BMP280」に6Byteのデータ要求(一括読み込み)
  for (i=0; i<6; i++){
    while (Wire.available() == 0 ){}//バッファに要求したデータがたまるまで待機
    dac[i] = Wire.read();//dacにI2Cデバイス「BMP280」のデータを格納(バッファ->配列dac)
  }
  
  //or演算でデータを合成
  //(有意なのは20bitでデータは上位から詰めるように8bitごとに3分割されている->最下位データで有効なのは上位4bitだけ)
  adc_P = ((uint32_t)dac[0] << 12) | ((uint32_t)dac[1] << 4) | ((dac[2] >> 4) & 0x0F); //これはデータ合成の応用的なもの
  adc_T = ((uint32_t)dac[3] << 12) | ((uint32_t)dac[4] << 4) | ((dac[5] >> 4) & 0x0F); //詳しくはBME280のデータシート7.4.7を見ればわかると思う 右シフトは最上位の複製つまり1001を右シフト最上位1が複製され11111001になってしまうだから最後に0x0F（00001111）とand演算がとられている（これをマスキングという）
  
  pres_cal = BMP280_compensate_P_int32(adc_P);//気圧データ補正計算 この関数についてはデータシートにこの処理をしなさいと書いてある。最後のほう。この補償計算に文句がある場合は自分たちで考えなければならない。今回は単位等がよくわからないためあんまいじらないほうがいい
  temp_cal = BMP280_compensate_T_int32(adc_T);//温度データ補正計算

  pres = (float)pres_cal / 100.0;//気圧データを実際の値に計算
  temp = (float)temp_cal / 100.0;//温度データを実際の値に計算

  //シリアルモニタ送信
  Serial.print("Pressure:");//文字列「Pressure:」をシリアルモニタに送信
  Serial.print(pres,2);//「pres」をシリアルモニタに送信
  Serial.print("hPa ");//文字列「hPa 」をシリアルモニタに送信
  Serial.print("Temp:");//文字列「Temp:」をシリアルモニタに送信
  Serial.print(temp,2);//「temp」をシリアルモニタに送信
  Serial.println("°C ");//文字列「°C 」をシリアルモニタに送信
  
  delay(1000);//1000msec待機(1秒待機)
}


//温度補正 関数
int32_t BMP280_compensate_T_int32(int32_t adc_T)
{
  int32_t var1, var2, T;
  var1  = ((((adc_T>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11;
  var2  = (((((adc_T>>4) - ((int32_t)dig_T1)) * ((adc_T>>4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  T  = (t_fine * 5 + 128) >> 8;
  return T;
}


//気圧補正 関数
uint32_t BMP280_compensate_P_int32(int32_t adc_P)
{
  int32_t var1, var2;
  uint32_t p;
  var1 = (((int32_t)t_fine)>>1) - (int32_t)64000;
  var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)dig_P6);
  var2 = var2 + ((var1*((int32_t)dig_P5))<<1);
  var2 = (var2>>2)+(((int32_t)dig_P4)<<16);
  var1 = (((dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int32_t)dig_P2) * var1)>>1))>>18;
  var1 =((((32768+var1))*((int32_t)dig_P1))>>15);
  if (var1 == 0)
  {
    return 0; // avoid exception caused by division by zero
  }
  p = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125;
  if (p < 0x80000000)
  {
    p = (p << 1) / ((uint32_t)var1);
  }
  else
  {
    p = (p / (uint32_t)var1) * 2;
  }
  var1 = (((int32_t)dig_P9) * ((int32_t)(((p>>3) * (p>>3))>>13)))>>12;
  var2 = (((int32_t)(p>>2)) * ((int32_t)dig_P8))>>13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
  return p;
}