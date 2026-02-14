#include <Arduino.h>
#include <driver/i2s.h>

// Конфигурация пинов (измените под вашу плату)
#define I2S_BCLK_PIN 14
#define I2S_LRC_PIN  15
#define I2S_DATA_PIN 32 // DIN для динамика или DOUT для микрофона
#define BLUE_LED 2

// Буфер для чтения данных
#define READ_BUFFER_SIZE 1024
uint8_t readBuffer[READ_BUFFER_SIZE];
// Таймер
int last_time = 0;
// Среднее значение амплитуды за 100 мс
float sum = 0, count = 4608;

void setupI2S() {
  // 1. Конфигурация I2S драйвера
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Уровень прерывания
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true, // Очищать буфер при отсутствии данных (тишина)
    .fixed_mclk = 0
  };

  // 2. Установка драйвера
  // I2S_NUM_0 - номер порта
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true);
  }

  // 3. Конфигурация пинов
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRC_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DATA_PIN
  };
  
  i2s_set_pin(I2S_NUM_0, &pin_config);
  
  
  
  Serial.println("I2S driver installed.");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  setupI2S();
  // Настройка пина светодиода
  pinMode(BLUE_LED, OUTPUT);
}

// put function declarations here:

void loop() {
  size_t bytesRead = 0;
  
  // Чтение блока данных из I2S
  esp_err_t result = i2s_read(I2S_NUM_0, &readBuffer, READ_BUFFER_SIZE, &bytesRead, portMAX_DELAY);
  
  if (result == ESP_OK && bytesRead > 0) {
    int samplesRead = bytesRead / sizeof(int16_t);
    int16_t* samples = (int16_t*)readBuffer;
    
    // Проходим по буферу и выводим значения
    // Шаг 2 (стерео, берем левый канал)
    for (int i = 0; i < samplesRead; i += 2) {
      // Вывод в Serial Plotter
      
      sum += abs(samples[i]);
      // Внимание: При высокой частоте дискретизации (>16кГц) 
      // вывод в Serial может замедлять цикл. 
      //Для теста лучше использовать меньшую частоту или выводить каждый N-й сэмпл.
    }

    Serial.print(">");
    Serial.print("var:");
    Serial.print(sum / count);
    Serial.print(",");
    Serial.println();

    if (millis() - last_time >= 100) {
      if (sum / count >= 1000) {
        digitalWrite(BLUE_LED, HIGH);
      }
      else {
        digitalWrite(BLUE_LED, LOW);
      }
      sum = 0;
      last_time = millis();
    }
  }
}

// put function definitions here: