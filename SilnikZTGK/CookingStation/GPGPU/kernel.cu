#define _CRT_SECURE_NO_WARNINGS
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

__global__ void processImageKernel(unsigned char* input, unsigned char* output, int width, int height, int channels, int parameter) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // Sprawdzenie granic obrazu
    if (x >= width || y >= height) return;

    int pixelIndex = (y * width + x) * channels;


    // ==========================================
    // ADRIAN: SOBEL (Wykrywanie krawędzi)
    // parameter: 0 = maska pozioma, 1 = maska pionowa
    // ==========================================

    int Gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    int Gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

    for (int c = 0; c < channels; c++) {
        // Pomijamy kanał Alpha (przezroczystość), żeby obrazek nie zniknął
        if (channels == 4 && c == 3) {
            output[pixelIndex + c] = input[pixelIndex + c];
            continue;
        }

        int sum = 0;
        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                // Zabezpieczenie przed wyjściem poza krawędzie obrazka (clamping)
                int nx = max(0, min(x + kx, width - 1));
                int ny = max(0, min(y + ky, height - 1));

                int val = input[(ny * width + nx) * channels + c];
                int weight = (parameter == 0) ? Gx[ky + 1][kx + 1] : Gy[ky + 1][kx + 1];
                sum += val * weight;
            }
        }
        sum = abs(sum);
        output[pixelIndex + c] = min(255, sum);
    }


    // ==========================================
    // OLA: DILATION (Dylatacja - pogrubianie jasnych)
    // parameter: rozmiar maski (np. 3 dla 3x3, 5 dla 5x5)
    // ==========================================
    
    //int radius = parameter / 2;
    //for (int c = 0; c < channels; c++) {
    //    if (channels == 4 && c == 3) {
    //        output[pixelIndex + c] = input[pixelIndex + c];
    //        continue;
    //    }

    //    int maxVal = 0; // Szukamy najjaśniejszego piksela
    //    for (int ky = -radius; ky <= radius; ky++) {
    //        for (int kx = -radius; kx <= radius; kx++) {
    //            int nx = max(0, min(x + kx, width - 1));
    //            int ny = max(0, min(y + ky, height - 1));

    //            int val = input[(ny * width + nx) * channels + c];
    //            if (val > maxVal) maxVal = val;
    //        }
    //    }
    //    output[pixelIndex + c] = maxVal;
    //}
    

    // ==========================================
    // AMELIA: EROSION (Erozja - zjadanie jasnych/pogrubianie ciemnych)
    // parameter: rozmiar maski (np. 3 dla 3x3, 5 dla 5x5)
    // ==========================================
 
    //int radius = parameter / 2;
    //for (int c = 0; c < channels; c++) {
    //    if (channels == 4 && c == 3) {
    //        output[pixelIndex + c] = input[pixelIndex + c];
    //        continue;
    //    }

    //    int minVal = 255; // Szukamy najciemniejszego piksela
    //    for (int ky = -radius; ky <= radius; ky++) {
    //        for (int kx = -radius; kx <= radius; kx++) {
    //            int nx = max(0, min(x + kx, width - 1));
    //            int ny = max(0, min(y + ky, height - 1));

    //            int val = input[(ny * width + nx) * channels + c];
    //            if (val < minVal) minVal = val;
    //        }
    //    }
    //    output[pixelIndex + c] = minVal;
    //}
   

    // ==========================================
    // OSKAR: MIN/MAX FILTER (Filtr minimalny lub maksymalny)
    // parameter: 0 = szuka minimum, 1 = szuka maksimum. (Maska na sztywno 3x3)
    // ==========================================
    
    for (int c = 0; c < channels; c++) {
        if (channels == 4 && c == 3) {
            output[pixelIndex + c] = input[pixelIndex + c];
            continue;
        }

        int extremeVal = (parameter == 0) ? 255 : 0;
        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int nx = max(0, min(x + kx, width - 1));
                int ny = max(0, min(y + ky, height - 1));

                int val = input[(ny * width + nx) * channels + c];

                if (parameter == 0 && val < extremeVal) extremeVal = val; // Min
                if (parameter == 1 && val > extremeVal) extremeVal = val; // Max
            }
        }
        output[pixelIndex + c] = extremeVal;
    }
    
}
// =================================================================

void runProcessing(unsigned char* h_input, int width, int height, int channels, int parameter, const char* out_filename) {
    size_t imgSize = width * height * channels * sizeof(unsigned char);
    unsigned char* d_input, * d_output;
    unsigned char* h_output = new unsigned char[width * height * channels];

    // 1. Alokacja pamięci na GPU
    cudaMalloc((void**)&d_input, imgSize);
    cudaMalloc((void**)&d_output, imgSize);

    // 2. Kopiowanie obrazu z RAM (Host) do VRAM (Device)
    cudaMemcpy(d_input, h_input, imgSize, cudaMemcpyHostToDevice);

    // 3. Konfiguracja siatki wątków (Bloki 16x16)
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    // 4. Uruchomienie algorytmu na GPU
    processImageKernel << <gridSize, blockSize >> > (d_input, d_output, width, height, channels, parameter);
    cudaDeviceSynchronize();

    // 5. Pobranie wyniku z GPU z powrotem do RAM
    cudaMemcpy(h_output, d_output, imgSize, cudaMemcpyDeviceToHost);

    // 6. Zapis do pliku PNG
    stbi_write_png(out_filename, width, height, channels, h_output, width * channels);
    std::cout << "Zapisano: " << out_filename << std::endl;

    // Sprzątanie
    cudaFree(d_input);
    cudaFree(d_output);
    delete[] h_output;
}

int main() {
    int width, height, channels;
    unsigned char* img = stbi_load("input.png", &width, &height, &channels, 0);

    if (img == NULL) {
        std::cerr << "Blad ladowania obrazu 'input.png'!" << std::endl;
        return -1;
    }

    std::cout << "Zaladowano obraz: " << width << "x" << height << " kanalow: " << channels << std::endl;

    // --- ADRIAN: SOBEL ---
    runProcessing(img, width, height, channels, 0, "output_sobel_poziomy.png");
    runProcessing(img, width, height, channels, 1, "output_sobel_pionowy.png");

    // --- OLA: DYLATACJA ---
    //runProcessing(img, width, height, channels, 3, "output_dylatacja_3x3.png");
    //runProcessing(img, width, height, channels, 9, "output_dylatacja_9x9.png");

    // --- AMELIA: EROZJA ---
    //runProcessing(img, width, height, channels, 3, "output_erozja_3x3.png");
    //runProcessing(img, width, height, channels, 9, "output_erozja_9x9.png");

    // --- OSKAR: MIN/MAX FILTER ---
    //runProcessing(img, width, height, channels, 0, "output_filtr_min.png");
    //runProcessing(img, width, height, channels, 1, "output_filtr_max.png");

    stbi_image_free(img);
    return 0;
}