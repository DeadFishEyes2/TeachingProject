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

void handle_input(char **symbol, char **range, char **file_name,
                  int *K, double *P_start, double *P_target) {
    static int i = 15;
    i++;
    char buffer[100];
    char default_file[25];

    printf("Enter stock symbol (default = AAPL): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    *symbol = (strlen(buffer) == 0) ? strdup("AAPL") : strdup(buffer);

    printf("Enter range (ex: 10d, 1mo) (default = 10d): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    *range = (strlen(buffer) == 0) ? strdup("10d") : strdup(buffer);

    printf("Enter number of days K: ");
    fgets(buffer, sizeof(buffer), stdin);
    *K = atoi(buffer);

    printf("Enter starting price P_start: ");
    fgets(buffer, sizeof(buffer), stdin);
    *P_start = atof(buffer);

    printf("Enter target price P_target: ");
    fgets(buffer, sizeof(buffer), stdin);
    *P_target = atof(buffer);

    printf("Enter file name (default = data%d.in): ", i);
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    if (strlen(buffer) == 0) {
        sprintf(default_file, "data%d.in", i);
        *file_name = strdup(default_file);
    } else {
        *file_name = strdup(buffer);
    }
}

int add_another_file() {
    char buffer[100];
    printf("\nCreate another file? y/n (default = n): ");
    if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (buffer[0] == 'y' || buffer[0] == 'Y') return 1;
    }
    return 0;
}

void write_test_file(const char* file_name, double* prices, int count,
                     int K, double P_start, double P_target) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "input/%s", file_name);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        printf("Error: Could not create file %s\n", full_path);
        return;
    }

    fprintf(f, "%d\n", count);
    fprintf(f, "1.0\n");
    fprintf(f, "%d\n", K);
    fprintf(f, "%.2f\n", P_start);
    fprintf(f, "%.2f\n", P_target);

    for (int i = 0; i < count; i++) {
        if (prices[i] >= 0)
            fprintf(f, "%.2f", prices[i]);
        else
            fprintf(f, "0.00"); // shouldn't happen as long as there aren't any bugs in the api
        if (i < count - 1)
            fprintf(f, "\n");
    }

    fclose(f);
    printf("File %s was generated!\n", full_path);
}

void get_and_write_prices(const char* symbol, const char* range, const char* file_name,
                          int K, double P_start, double P_target) {
    int count = 0;
    double* prices = get_open_prices(symbol, "1d", range, &count);

    if (K >= count) {
        printf("Warning: K=%d must be less than N=%d. Adjusting K to %d.\n", K, count, count - 1);
        K = count - 1;
    }

    write_test_file(file_name, prices, count, K, P_start, P_target);
    free(prices);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    char* symbol = NULL;
    char* range = NULL;
    char* file_name = NULL;
    double P_start, P_target;
    int K;
    int ok = 0;

    do {
        handle_input(&symbol, &range, &file_name, &K, &P_start, &P_target);
        get_and_write_prices(symbol, range, file_name, K, P_start, P_target);
        free(symbol);
        free(range);
        free(file_name);
        symbol = range = file_name = NULL;
        ok = add_another_file();
    } while (ok);

    curl_global_cleanup();
    return 0;
}