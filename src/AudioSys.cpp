#include "AudioSys.h"
#include "game_defs.h"
#include <SD.h>
#include <driver/i2s.h>

// --- AUDIO GLOBALS ---
uint8_t* voiceBuffer = nullptr;
size_t voiceSize = 0;
float soundVolume = 0.5; 
const int DEFAULT_RATE = 16000; // Default sample rate for text voice

// Helper: Applies volume and fades to the buffer PERMANENTLY upon load.
void processAudioBuffer(uint8_t* buffer, size_t size) {
    if (!buffer) return;

    int16_t* samples = (int16_t*)buffer;
    size_t sampleCount = size / 2;
    const int fadeSamples = 150; 

    for (size_t i = 0; i < sampleCount; i++) {
        float fade = 1.0f;
        if (i < fadeSamples) fade = (float)i / (float)fadeSamples;
        else if (i > sampleCount - fadeSamples) fade = (float)(sampleCount - i) / (float)fadeSamples;

        samples[i] = (int16_t)(samples[i] * soundVolume * fade);
    }
}

// Loads a file into a raw buffer (Caller must free it!)
// Used for short, repetitive sounds like Voice
uint8_t* loadRawFile(const char* filename, size_t& outSize) {
    File file = SD.open(filename);
    if (!file) {
        Serial.printf("Audio not found: %s\n", filename);
        outSize = 0;
        return nullptr;
    }
    if (file.size() <= 44) { file.close(); return nullptr; }
    
    size_t dataSize = file.size() - 44;
    file.seek(44); // Skip header

    if (ESP.getFreeHeap() < dataSize) {
        Serial.printf("Not enough RAM for %s\n", filename);
        file.close();
        return nullptr;
    }

    uint8_t* buff = (uint8_t*)malloc(dataSize);
    if (buff) {
        file.read(buff, dataSize);
    }
    file.close();
    outSize = dataSize;
    return buff;
}

void setVoice(const char* filename) {
    if (voiceBuffer) {
        free(voiceBuffer);
        voiceBuffer = nullptr;
    }
    voiceBuffer = loadRawFile(filename, voiceSize);
    if (voiceBuffer) {
        processAudioBuffer(voiceBuffer, voiceSize);
        Serial.printf("Voice set to %s\n", filename);
    }
}

void setupAudio() {
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Start Failed");
        return;
    }

    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = DEFAULT_RATE, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo Mode
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

    // Load default voice
    setVoice("/text.wav");
}

void playVoice() {
    if (!voiceBuffer) return;
    size_t bytes_written;
    // Assumes Voice file matches the default I2S rate (16k)
    i2s_write(I2S_NUM_0, voiceBuffer, voiceSize, &bytes_written, portMAX_DELAY);
}

// --- STREAMING PLAYER FOR SFX (DIALUP) ---
// This reads from SD card in chunks, avoiding malloc failures.
// It also reads the WAV header to adjust playback speed automatically.
void playSFX(const char* filename) {
    File file = SD.open(filename);
    if (!file) {
        Serial.printf("SFX missing: %s\n", filename);
        return;
    }

    // 1. PARSE WAV HEADER
    uint8_t header[44];
    if (file.read(header, 44) != 44) { file.close(); return; }

    // Offset 22: Channels (2 bytes)
    uint16_t channels = header[22] | (header[23] << 8);
    // Offset 24: Sample Rate (4 bytes)
    uint32_t sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);

    // Safety fallback
    if (sampleRate == 0) sampleRate = 16000;
    
    // Temporarily change I2S rate to match this file
    i2s_set_sample_rates(I2S_NUM_0, sampleRate);

    // 2. STREAMING BUFFER
    const size_t chunkSize = 512; 
    uint8_t readBuf[chunkSize];
    // Output buffer is larger to handle Mono->Stereo expansion if needed
    uint8_t outBuf[chunkSize * 2]; 

    while (file.available()) {
        size_t bytesRead = file.read(readBuf, chunkSize);
        
        int16_t* rawSamples = (int16_t*)readBuf;
        int16_t* finalSamples = (int16_t*)outBuf;
        int sampleCount = bytesRead / 2; // Assuming 16-bit input
        
        size_t bytesToWrite = 0;

        // 3. CONVERT / PROCESS
        if (channels == 1) {
            // MONO -> STEREO EXPANSION
            // We must duplicate every sample L -> L/R
            for (int i = 0; i < sampleCount; i++) {
                int16_t val = (int16_t)(rawSamples[i] * soundVolume);
                finalSamples[2*i] = val;     // Left
                finalSamples[2*i + 1] = val; // Right
            }
            bytesToWrite = sampleCount * 4; // 2 channels * 2 bytes
        } else {
            // STEREO DIRECT
            for (int i = 0; i < sampleCount; i++) {
                finalSamples[i] = (int16_t)(rawSamples[i] * soundVolume);
            }
            bytesToWrite = bytesRead;
        }

        // 4. WRITE TO I2S
        size_t bytesWritten;
        i2s_write(I2S_NUM_0, outBuf, bytesToWrite, &bytesWritten, portMAX_DELAY);
    }

    file.close();

    // 5. RESTORE DEFAULT RATE (For Voice)
    i2s_set_sample_rates(I2S_NUM_0, DEFAULT_RATE);
}