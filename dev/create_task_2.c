#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"
#include <time.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

double* get_open_prices(const char* symbol, const char* interval, const char* range, int *count_out) {
    CURL *curl_handle;
    struct MemoryStruct chunk = {malloc(1), 0};
    char url[256];
    double *prices = NULL;

    sprintf(url, "https://query1.finance.yahoo.com/v8/finance/chart/%s?interval=%s&range=%s", symbol, interval, range);
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    if (curl_easy_perform(curl_handle) == CURLE_OK) {
        cJSON *json = cJSON_Parse(chunk.memory);
        // Navigare ierarhie JSON
        cJSON *result = cJSON_GetArrayItem(cJSON_GetObjectItem(cJSON_GetObjectItem(json, "chart"), "result"), 0);
        cJSON *quote = cJSON_GetArrayItem(cJSON_GetObjectItem(cJSON_GetObjectItem(result, "indicators"), "quote"), 0);
        cJSON *open_prices = cJSON_GetObjectItem(quote, "open");

        *count_out = cJSON_GetArraySize(open_prices);
        prices = malloc((*count_out) * sizeof(double));

        for (int i = 0; i < *count_out; i++) {
            cJSON *val = cJSON_GetArrayItem(open_prices, i);
            prices[i] = cJSON_IsNumber(val) ? val->valuedouble : -1.0; 
        }
        cJSON_Delete(json);
    }

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    return prices;
}

void handle_input(char **symbol, char **range, char **file_name) {
    static int i = 10;
    i++;
    char buffer[100];
    char default_file[20];
    

    printf("Introdu simbolul acțiunii (default = AAPL): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0) 
        *symbol = strdup("AAPL");
    else 
        *symbol = strdup(buffer);

    printf("Introdu intervalul (ex: 10d, 1mo) (default = 10d): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0) 
        *range = strdup("10d");
    else 
        *range = strdup(buffer);

    printf("Introdu numele fișierului de ieșire (default = data%d.in) ", i);
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0){
        sprintf(default_file, "data%d.in", i);
        *file_name = strdup(default_file);
    } else 
        *file_name = strdup(buffer);
}

int add_another_file() {
    char buffer[100];

    printf("\nCreate another file? y/n (default = n): ");
    if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0 || buffer[0] == 'n' || buffer[0] == 'N') {
            return 0;
        }

        if (buffer[0] == 'y' || buffer[0] == 'Y') {
            return 1;
        }
    }

    return 0;
}

void write_test_file(const char* file_name, double* prices, int count) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "input/%s", file_name);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        printf("Eroare: Nu s-a putut crea fisierul %s\n", full_path);
        return;
    }

    fprintf(f, "%d\n", count);

    srand(time(NULL));
    for (int i = 0; i < count; i++) {
        double x = prices[i];
        if (x < 0) x = 100.0;

        double i1 = x;
        double i2 = 2 * x;
        double i3 = x + 5;

        fprintf(f, "%.2f %.2f %.2f", i1, i2, i3);
        if (i < count-1)
            fprintf(f, "\n");
    }

    fclose(f);
    printf("Fișierul pentru Task 2 %s a fost generat.\n", full_path);
}

void get_and_write_prices(const char* symbol, const char* range, const char* file_name){
    int count = 0;
    double* prices = get_open_prices(symbol, "1d", range, &count);
    write_test_file(file_name, prices, count);
    free(prices);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    
    char* symbol = NULL;
    char* range = NULL;
    char* file_name = NULL;
    int ok = 0;

    do {
        handle_input(&symbol, &range, &file_name);
        get_and_write_prices(symbol, range, file_name);
        ok = add_another_file();
    } while (ok);

    free(symbol);
    free(range);

    curl_global_cleanup();
    return 0;
}