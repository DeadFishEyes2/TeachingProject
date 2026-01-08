#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

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

void get_open_prices(const char* symbol, const char* interval, const char* range) {
    CURL *curl_handle;
    struct MemoryStruct chunk = {malloc(1), 0};
    char url[256];

    sprintf(url, "https://query1.finance.yahoo.com/v8/finance/chart/%s?interval=%s&range=%s", symbol, interval, range);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    if (curl_easy_perform(curl_handle) == CURLE_OK) {
        // --- PARSING JSON ---
        cJSON *json = cJSON_Parse(chunk.memory);
        cJSON *chart = cJSON_GetObjectItem(json, "chart");
        cJSON *result_array = cJSON_GetObjectItem(chart, "result");
        cJSON *result = cJSON_GetArrayItem(result_array, 0);
        cJSON *indicators = cJSON_GetObjectItem(result, "indicators");
        cJSON *quote_array = cJSON_GetObjectItem(indicators, "quote");
        cJSON *quote = cJSON_GetArrayItem(quote_array, 0);
        cJSON *open_prices = cJSON_GetObjectItem(quote, "open");

        int count = cJSON_GetArraySize(open_prices);
        double *prices = malloc(count * sizeof(double));

        printf("Preturi deschidere pentru %s (%d zile):\n", symbol, count);
        for (int i = 0; i < count; i++) {
            cJSON *val = cJSON_GetArrayItem(open_prices, i);
            if (cJSON_IsNumber(val)) {
                prices[i] = val->valuedouble;
                printf("[%d]: %.2f\n", i, prices[i]);
            }
        }

        free(prices);
        cJSON_Delete(json);
    }

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
}

void handle_input(char **symbol, char **range) {
    char buffer[100];

    printf("Introdu simbolul acțiunii (default = AAPL): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0) *symbol = strdup("AAPL");
    else *symbol = strdup(buffer);

    printf("Introdu intervalul (ex: 10d, 1mo) (default = 10d): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0) *range = strdup("10d");
    else *range = strdup(buffer);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    
    char* symbol = NULL;
    char* range = NULL;
    
    handle_input(&symbol, &range);

    get_open_prices(symbol, "1d", range);
    
    free(symbol);
    free(range);

    curl_global_cleanup();
    return 0;
}