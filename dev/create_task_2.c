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

// Ask user for market names and how many days each market should have
void handle_markets(char market_names[3][100], int counts[3], int max_count) {
    printf("\nMaxim disponibil: %d zile\n", max_count);

    for (int m = 0; m < 3; m++) {
        char buffer[100];

        printf("Numele pietei %d (default = Piata%d): ", m + 1, m + 1);
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) == 0)
            snprintf(market_names[m], 100, "Piata%d", m + 1);
        else
            strncpy(market_names[m], buffer, 100);

        printf("Numar de zile pentru %s (max %d, default = %d): ", market_names[m], max_count, max_count);
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0) {
            counts[m] = max_count;
        } else {
            counts[m] = atoi(buffer);
            if (counts[m] <= 0 || counts[m] > max_count)
                counts[m] = max_count;
        }
    }
}

void handle_input(char **symbol, char **range, char **file_name) {
    static int i = 5;
    i++;
    char buffer[100];
    char default_file[20];

    printf("Introdu simbolul actiunii (default = AAPL): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    *symbol = (strlen(buffer) == 0) ? strdup("AAPL") : strdup(buffer);

    printf("Introdu intervalul (ex: 10d, 1mo, 6mo, 1y) (default = 6mo): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    *range = (strlen(buffer) == 0) ? strdup("6mo") : strdup(buffer);

    printf("Introdu numele fisierului de iesire (default = data%d.in): ", i);
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
        if (strlen(buffer) == 0 || buffer[0] == 'n' || buffer[0] == 'N') return 0;
        if (buffer[0] == 'y' || buffer[0] == 'Y') return 1;
    }
    return 0;
}

// Returns value with optional +5% noise based on noise_chance (0-100)
double apply_noise(double x, int noise_chance) {
    if ((rand() % 100) < noise_chance)
        return x * 1.05;
    return x;
}

void write_market_block(FILE *f, const char* name, double* prices, int count, int noise_chance) {
    fprintf(f, "%s\n", name);
    for (int i = count - 1; i >= 0; i--) {
        double val = prices[i] > 0 ? prices[i] : 100.0;
        fprintf(f, "%.2f\n", apply_noise(val, noise_chance));
    }
}

void write_test_file(const char* file_name, double* prices,
                     char market_names[3][100], int counts[3], int noise_chance) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "input/%s", file_name);

    FILE *f = fopen(full_path, "w");
    if (!f) {
        printf("Eroare: Nu s-a putut crea fisierul %s\n", full_path);
        return;
    }

    // Each market takes its slice from the front of the shared price list
    for (int m = 0; m < 3; m++) {
        write_market_block(f, market_names[m], prices, counts[m], noise_chance);
    }

    fclose(f);
    printf("Fisierul pentru Task 2 %s a fost generat.\n", full_path);
}

void get_and_write_prices(const char* symbol, const char* range, const char* file_name) {
    int total_count = 0;
    double* prices = get_open_prices(symbol, "1d", range, &total_count);

    if (!prices || total_count == 0) {
        printf("Eroare: Nu s-au putut obtine preturile.\n");
        return;
    }

    char market_names[3][100];
    int counts[3];
    handle_markets(market_names, counts, total_count);

    char buffer[100];
    int noise_chance = 10; // default 10%
    printf("Probabilitate de zgomot +5%% pe zi per piata (0-100, default = 10): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    if (strlen(buffer) > 0) {
        int n = atoi(buffer);
        if (n >= 0 && n <= 100) noise_chance = n;
    }

    write_test_file(file_name, prices, market_names, counts, noise_chance);
    free(prices);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    srand(time(NULL));

    char* symbol    = NULL;
    char* range     = NULL;
    char* file_name = NULL;
    int ok = 0;

    do {
        handle_input(&symbol, &range, &file_name);
        get_and_write_prices(symbol, range, file_name);
        ok = add_another_file();

        free(symbol); free(range); free(file_name);
        symbol = range = file_name = NULL;
    } while (ok);

    curl_global_cleanup();
    return 0;
}