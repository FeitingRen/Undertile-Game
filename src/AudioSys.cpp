#include "AudioSys.h"
#include "game_defs.h"
#include <SD.h>
#include <driver/i2s.h>

// --- AUDIO GLOBALS ---
uint8_t* audioBuffer = nullptr;
size_t audioBufferSize = 0;
bool audioLoaded = false;
float soundVolume = 0.5; // Range: 0.0 (Mute) to 1.0 (Max)

// --- AUDIO FUNCTIONS ---
void setupAudio() {
    // 1. Initialize SD Card
    // Note: SD uses SPI. Ensure SD_CS (21) is correct.
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Start Failed");
        return;
    }

    // 2. Load "text.wav" into RAM
    File file = SD.open("/text.wav");
    if (!file) {
        Serial.println("text.wav not found!");
        return;
    }

    // Read WAV Header to get Sample Rate
    uint8_t header[44];
    file.read(header, 44);
    
    // Extract Sample Rate (Offset 24, 4 bytes, little endian)
    uint32_t sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    uint16_t channels = header[22] | (header[23] << 8);
    
    Serial.printf("WAV: %d Hz, %d Channels\n", sampleRate, channels);

    audioBufferSize = file.size() - 44; // Total size minus header
    audioBuffer = (uint8_t*)malloc(audioBufferSize); // Allocate RAM
    
    if (audioBuffer) {
        file.read(audioBuffer, audioBufferSize); // Copy file to RAM
        audioLoaded = true;
        Serial.println("Audio Loaded to RAM");
    } else {
        Serial.println("Not enough RAM for audio!");
    }
    file.close();

    // 3. Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = (channels == 2) ? I2S_CHANNEL_FMT_RIGHT_LEFT : I2S_CHANNEL_FMT_ONLY_LEFT,
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
}

void playTextSound() {
    if (!audioLoaded || !audioBuffer) return;

    size_t bytes_written;
    
    // We process the audio in small chunks (256 bytes = 128 samples)
    // This keeps the memory usage low and the loop fast.
    uint8_t tempBuffer[256]; 
    size_t chunk_size = sizeof(tempBuffer);

    for (size_t i = 0; i < audioBufferSize; i += chunk_size) {
        // 1. Calculate how many bytes remain
        size_t bytes_to_process = (audioBufferSize - i) < chunk_size ? (audioBufferSize - i) : chunk_size;

        // 2. Copy the raw original audio into our temp buffer
        memcpy(tempBuffer, &audioBuffer[i], bytes_to_process);

        // 3. Apply Volume Scaling
        // We cast the buffer to int16_t* because your WAV is 16-bit
        int16_t* samples = (int16_t*)tempBuffer;
        size_t sample_count = bytes_to_process / 2; // 2 bytes per sample

        for (size_t s = 0; s < sample_count; s++) {
            // Multiply the sample by the volume factor
            samples[s] = (int16_t)(samples[s] * soundVolume);
        }

        // 4. Send the modified chunk to the I2S Driver
        i2s_write(I2S_NUM_0, tempBuffer, bytes_to_process, &bytes_written, portMAX_DELAY);
    }
}
