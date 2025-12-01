#include "AudioSys.h"
#include "game_defs.h"
#include <SD.h>
#include <driver/i2s.h>

// --- AUDIO GLOBALS ---
uint8_t* voiceBuffer = nullptr;
size_t voiceSize = 0;
float soundVolume = 0.5; 
const int DEFAULT_RATE = 16000;

// --- STREAMING GLOBALS ---
File sfxFile;
bool sfxPlaying = false;
uint8_t sfxBuffer[512]; // Small buffer for streaming
uint32_t sfxSampleRate = 16000;
uint16_t sfxChannels = 1;

void processAudioBuffer(uint8_t* buffer, size_t size) {
    if (!buffer) return;
    int16_t* samples = (int16_t*)buffer;
    size_t sampleCount = size / 2;
    for (size_t i = 0; i < sampleCount; i++) {
        samples[i] = (int16_t)(samples[i] * soundVolume);
    }
}

uint8_t* loadRawFile(const char* filename, size_t& outSize) {
    File file = SD.open(filename);
    if (!file) { outSize = 0; return nullptr; }
    if (file.size() <= 44) { file.close(); return nullptr; }
    
    size_t dataSize = file.size() - 44;
    file.seek(44);

    if (ESP.getFreeHeap() < dataSize) { file.close(); return nullptr; }

    uint8_t* buff = (uint8_t*)malloc(dataSize);
    if (buff) file.read(buff, dataSize);
    
    file.close();
    outSize = dataSize;
    return buff;
}

void setVoice(const char* filename) {
    if (voiceBuffer) { free(voiceBuffer); voiceBuffer = nullptr; }
    voiceBuffer = loadRawFile(filename, voiceSize);
    if (voiceBuffer) {
        processAudioBuffer(voiceBuffer, voiceSize);
    }
}

void setupAudio() {
    if (!SD.begin(SD_CS)) return;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = DEFAULT_RATE, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);

    setVoice("/text.wav");
}

void playVoice() {
    if (!voiceBuffer) return;
    size_t bytes_written;
    // FIX: increased timeout from 0 to 2 ticks. 
    // This allows the buffer a tiny moment to accept data if it's busy,
    // reducing "dropped" beeps during fast text.
    i2s_write(I2S_NUM_0, voiceBuffer, voiceSize, &bytes_written, 2);
}

// --- NEW NON-BLOCKING STREAMING ---

void startSFX(const char* filename) {
    stopSFX(); // Stop any previous sound

    sfxFile = SD.open(filename);
    if (!sfxFile) return;

    // Parse Header
    uint8_t header[44];
    if (sfxFile.read(header, 44) != 44) { stopSFX(); return; }

    sfxChannels = header[22] | (header[23] << 8);
    sfxSampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    if (sfxSampleRate == 0) sfxSampleRate = 16000;

    // Update hardware speed
    i2s_set_sample_rates(I2S_NUM_0, sfxSampleRate);
    
    sfxPlaying = true;
}

void updateSFX() {
    if (!sfxPlaying || !sfxFile.available()) {
        if (sfxPlaying && !sfxFile.available()) stopSFX(); // Auto-stop at end
        return;
    }

    // Read a small chunk
    size_t bytesRead = sfxFile.read(sfxBuffer, sizeof(sfxBuffer));
    if (bytesRead == 0) return;

    // Prepare Output Buffer (Handle Mono -> Stereo)
    uint8_t outBuf[1024]; // Max size needed if expanding 512 bytes mono -> stereo
    int16_t* input = (int16_t*)sfxBuffer;
    int16_t* output = (int16_t*)outBuf;
    size_t bytesToSend = 0;
    int sampleCount = bytesRead / 2;

    if (sfxChannels == 1) {
        for (int i = 0; i < sampleCount; i++) {
            int16_t val = (int16_t)(input[i] * soundVolume);
            output[2*i] = val;     
            output[2*i+1] = val; 
        }
        bytesToSend = sampleCount * 4;
    } else {
        for (int i = 0; i < sampleCount; i++) {
            output[i] = (int16_t)(input[i] * soundVolume);
        }
        bytesToSend = bytesRead;
    }

    // Write to I2S (Non-blocking / Short timeout)
    size_t bytesWritten;
    // We use portMAX_DELAY to ensure smooth audio, BUT we rely on 
    // the calling loop (typeText) being fast enough to keep this fed.
    // If we used 0, audio might skip.
    i2s_write(I2S_NUM_0, outBuf, bytesToSend, &bytesWritten, 10); 
}

void stopSFX() {
    if (sfxFile) sfxFile.close();
    sfxPlaying = false;
    // Restore voice default rate
    i2s_set_sample_rates(I2S_NUM_0, DEFAULT_RATE);
}

bool isSFXPlaying() {
    return sfxPlaying;
}

// Smart delay that keeps audio alive
void waitAndPump(int ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        updateSFX();
        // Give ESP32 background tasks a chance to run
        vTaskDelay(1); 
    }
}