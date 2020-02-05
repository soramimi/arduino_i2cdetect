#include <Wire.h>

#define LED_PIN 13

// GPIO pin for I2C
#define I2C_SCL_PIN 10
#define I2C_SDA_PIN 11

void usleep(int us)
{
  delayMicroseconds(us);
}

// software I2C
class I2C {
private:
  void delay()
  {
    usleep(0);
  }

  // 初期化
  void init_i2c()
  {
    pinMode(I2C_SCL_PIN, INPUT_PULLUP); 
    pinMode(I2C_SDA_PIN, INPUT_PULLUP); 
  }

  void i2c_cl_0()
  {
    pinMode(I2C_SCL_PIN, OUTPUT); 
    digitalWrite(I2C_SCL_PIN, LOW);
  }

  void i2c_cl_1()
  {
    digitalWrite(I2C_SCL_PIN, HIGH);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP); 
  }

  void i2c_da_0()
  {
    pinMode(I2C_SDA_PIN, OUTPUT); 
    digitalWrite(I2C_SDA_PIN, LOW);
  }

  void i2c_da_1()
  {
    digitalWrite(I2C_SDA_PIN, HIGH);
    pinMode(I2C_SDA_PIN, INPUT_PULLUP); 
  }

  int i2c_get_da()
  {
    i2c_da_1();
    return digitalRead(I2C_SDA_PIN) ? 1 : 0;
  }

  // スタートコンディション
  void i2c_start()
  {
    i2c_da_0(); // SDA=0
    delay();
    i2c_cl_0(); // SCL=0
    delay();
  }

  // ストップコンディション
  void i2c_stop()
  {
    i2c_cl_1(); // SCL=1
    delay();
    i2c_da_1(); // SDA=1
    delay();
  }

  // リピーテッドスタートコンディション
  void i2c_repeat()
  {
    i2c_cl_1(); // SCL=1
    delay();
    i2c_da_0(); // SDA=0
    delay();
    i2c_cl_0(); // SCL=0
    delay();
  }

  // 1バイト送信
  bool i2c_write(int c)
  {
    int i;
    bool nack;

    delay();

    // 8ビット送信
    for (i = 0; i < 8; i++) {
      if (c & 0x80) {
        i2c_da_1(); // SCL=1
      } else {
        i2c_da_0(); // SCL=0
      }
      c <<= 1;
      delay();
      i2c_cl_1(); // SCL=1
      delay();
      i2c_cl_0(); // SCL=0
      delay();
    }

    i2c_da_1(); // SDA=1
    delay();

    i2c_cl_1(); // SCL=1
    delay();
    // NACKビットを受信
    nack = i2c_get_da();
    i2c_cl_0(); // SCL=0

    return nack;
  }

  // 1バイト受信
  int i2c_read(bool nack)
  {
    int i, c;

    i2c_da_1(); // SDA=1
    delay();

    c = 0;

    for (i = 0; i < 8; i++) {
      i2c_cl_1(); // SCL=1
      delay();
      c <<= 1;
      if (i2c_get_da()) { // SDAから1ビット受信
        c |= 1;
      }
      i2c_cl_0(); // SCL=0
      delay();
    }

    // NACKビットを送信
    if (nack) {
      i2c_da_1(); // SDA=1
    } else {
      i2c_da_0(); // SDA=0
    }
    delay();
    i2c_cl_1(); // SCL=1
    delay();
    i2c_cl_0(); // SCL=0
    delay();

    return c;
  }

  int address; // I2Cデバイスアドレス

public:
  I2C(int address)
    : address(address)
  {
    init_i2c();
  }

  // デバイスのレジスタを読み取る
  int scan(int addr)
  {
    int data;
    i2c_start();                   // スタート
    bool nack = i2c_write(addr << 1);       // デバイスアドレスを送信
    i2c_stop();                    // 受信
    return !nack;
  }

  // デバイスのレジスタに書き込む
  void write(int reg, int data)
  {
    i2c_start();                   // スタート
    i2c_write(address << 1);       // デバイスアドレスを送信
    if (reg >= 0) {
      i2c_write(reg);                // レジスタ番号を送信
    }
    i2c_write(data);               // データを送信
    i2c_stop();                    // ストップ
  }

  // デバイスのレジスタを読み取る
  int read(int reg)
  {
    int data;
    i2c_start();                   // スタート
    i2c_write(address << 1);       // デバイスアドレスを送信
    i2c_write(reg);                // レジスタ番号を送信
    i2c_repeat();                  // リピーテッドスタートコンディション
    i2c_write((address << 1) | 1); // デバイスアドレスを送信（読み取りモード）
    data = i2c_read(true);         // データを受信
    i2c_stop();                    // 受信
    return data;
  }
};

I2C *i2c;

void setup()
{
  i2c = new I2C(0);
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.print('\n');
  
  for (int i = 0; i < 128; i++) {
    bool ack = i2c->scan(i);
    if (ack) {
      char tmp[3];
      sprintf(tmp, "%02x", i);
      Serial.print(tmp);
    } else {
      Serial.print("--");
    }
    Serial.print((i % 16 < 15) ? ' ' : '\n');
  }
}
