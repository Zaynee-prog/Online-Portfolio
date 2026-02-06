// Original Code created by: HAIL ZACKERY JOSHUA D. BONGANCISO
// SAZAKE GAS STATION
//...
//..
//.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <zint.h>
#include <curl/curl.h>
#include <conio.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif

/* =========================================================================
 * 1. FILE PATH CONFIGURATIONS
 * ========================================================================= */

#define BASE_BARCODE_PATH "C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\barcodes\\"
#define BASE_RECEIPT_PATH "C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\receipts\\"
#define BASE_DATA_PATH "C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\necessary\\"

#define CUSTOMER_FILE      BASE_DATA_PATH "customers.txt"
#define LOYALTY_FILE       BASE_DATA_PATH "loyalty.txt"
#define CUSTOMER_ID_FILE   BASE_DATA_PATH "customer_id.txt"
#define STOCK_FILE         BASE_DATA_PATH "gasoline_stock.txt"
#define STOCK_LOG_FILE     BASE_DATA_PATH "stock_transactions.txt"
#define ADMIN_FILE         BASE_DATA_PATH "admins.txt"
#define CASHIER_LOG_FILE   BASE_DATA_PATH "cashier_logs.txt"
#define ADMIN_LOG_FILE     BASE_DATA_PATH "admin_logs.txt"
#define PRICE_FILE         BASE_DATA_PATH "prices.txt"
#define SHIFT_REPORT_FILE  BASE_DATA_PATH "shift_reports.txt"

/* =========================================================================
 * 2. GLOBAL CONSTANTS & DEFINES
 * ========================================================================= */

#define MAX_ITEMS 20
#define MAX_CUSTOMERS 1394329
#define MAX_ATTEMPTS 5

/* --- FOLDER CREATION COMMANDS --- */
#define DATA_DIR_CMD_WIN "if not exist \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\necessary\" mkdir \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\necessary\""
#define RECEIPTS_DIR_CMD_WIN "if not exist \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\receipts\" mkdir \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\receipts\""
#define BARCODES_DIR_CMD_WIN "if not exist \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\barcodes\" mkdir \"C:\\Users\\Zackery\\OneDrive\\Documents\\CodeBlocks\\Barcode CPEP Final 2nd Sem\\outputs\\barcodes\""

/* Unix placeholders */
#define DATA_DIR_CMD_UNIX "mkdir -p necessary"
#define RECEIPTS_DIR_CMD_UNIX "mkdir -p receipts"
#define BARCODES_DIR_CMD_UNIX "mkdir -p barcodes"

#ifdef _WIN32
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif

#define LOYALTY_MIN_PURCHASES 10
#define LOYALTY_MIN_SPENT 10000.0
#define LOYALTY_EXPIRY_DAYS 60
#define LOYALTY_EXPIRY_SECONDS (LOYALTY_EXPIRY_DAYS * 24 * 60 * 60)

#define LOW_STOCK_THRESHOLD 50.0
#define DEFAULT_STOCK 100.0
#define MAX_STOCK 2000.0
#define MAX_TYPES 4

/* --- DELIVERY TIME SETTING --- */
#define DELIVERY_TIME_SECONDS 1800  /* 30 Minutes */

#define SUPPLIER_EMAIL "zackeryjoshua646@gmail.com"
#define SENDER_EMAIL   "hzjdbonganciso@universityofbohol.edu.ph"
#define SMTP_URL       "smtp://smtp.gmail.com:587"
#define EMAIL_USERNAME "hzjdbonganciso@universityofbohol.edu.ph"
#define EMAIL_PASSWORD "ropc wxvu qwjz yyxc"

#define TAX_RATE 0.12
#define SC_PWD_DISCOUNT 0.20 /* 20% Discount for Seniors/PWD */

const int FUEL_W = 24;
const int LITERS_W = 7;
const int PL_W = 11;
const int SUB_W = 12;

/* --- UPDATED COLOR PALETTE --- */
const char* RED     = "\x1b[91m";
const char* GREEN   = "\x1b[92m";
const char* YELLOW  = "\x1b[93m";
const char* BLUE    = "\x1b[94m";
const char* MAGENTA = "\x1b[95m";
const char* CYAN    = "\x1b[96m";
const char* WHITE   = "\x1b[97m";
const char* BOLD    = "\x1b[1m";
const char* RESET   = "\x1b[0m";

char lastReceiptFilename[256] = ""; /* Initialize as empty */

#define PAD "                          "
#define TABLE_PAD "          "

/* =========================================================================
 * 3. DATA STRUCTURES
 * ========================================================================= */

typedef struct {
    char name[64];
    double price;
} Fuel;

Fuel fuels[] = {
    {"Diesel", 0.00},
    {"Gasoline (Regular)", 0.00},
    {"Gasoline (Premium)", 0.00},
    {"Kerosine", 0.00}
};
int fuelCount = sizeof(fuels) / sizeof(fuels[0]);

typedef struct {
    int fuelIndex;
    double liters;
    double pricePerLiter;
    double subtotal;
} Item;

typedef struct {
    int id;
    char name[256];
    int purchaseCount;
    double totalSpent;
    unsigned long long loyaltyCode;
} Customer;

typedef struct {
    char name[32];
    int id;
    char password[32];
} Cashier;

Cashier cashiers[] = {
    {"Zack", 1001, "zack123"},
    {"Sarah", 1002, "sarah123"},
    {"Kevin", 1003, "kevin123"},
    {"Christian", 1004, "chris123"},
    {"Bayot", 1005,"SiAton"},
};
int cashierCount = sizeof(cashiers) / sizeof(cashiers[0]);

typedef struct {
    int id;
    double currentStock;
    time_t lastRestockTime;
    double pendingAmount;
    time_t deliveryArrivalTime;
} GasolineStock;

typedef struct {
    char username[32];
    char password[32];
} Admin;

/* --- SHIFT SESSION TRACKER --- */
typedef struct {
    int cashierID;
    char cashierName[32];
    time_t loginTime;
    double sessionLiters[4]; // Corresponds to the 4 fuel types
    double sessionCashExpected;
    int customersServed;
} ShiftSession;

/* =========================================================================
 * 4. FUNCTION PROTOTYPES
 * ========================================================================= */
void getPasswordInput(char* buffer, int maxLen);
void sanitizeInput(char* str);
void pauseMessage(void);
int getStrictInt(int min, int max);
double getStrictDouble(double min);
unsigned long long getStrictULL(void);
void clearScreen(void);
FILE* safeOpen(const char* path, const char* mode);
void openImageInViewer(const char* filename);
int hasNumbers(const char* str);
void showLoading(void);
void pauseAndReturnToMenu(void);

int sendEmailToSupplier(const char* customMessage);

void savePricesToFile(void);
void loadPricesFromFile(void);
int arePricesSet(void);

void logStockTransaction(int id, const char* fuelName, double amount, const char* type);
void logCashierActivity(const char* cashierName, const char* action);
void logAdminActivity(const char* adminName, const char* action);

void repairStockFile(void);
int saveStocksToFile(GasolineStock stocks[], int count);
void processPendingDeliveries(GasolineStock stocks[], int count);
int loadStocksFromFile(GasolineStock* stocks);
void viewGasolineStock(void);
void manualRestockMenu(const char* adminName);
void stockMenu(void);
int findStockIndexByID(GasolineStock stocks[], int count, int id);
void deductStock(const char* fuelName, double litersSold);
int checkStockForPurchase(const char* fuelName, double litersNeeded);
void generateBarcodeImage(const char* data, const char* filename);
/* Updated Prototype for Senior Logic */
void printReceiptFormatted(FILE* out, const char* cashierName, int customerID, const char* customer, Item* items, int itemCount, double subtotal, double tax, double discount, double grandTotal, double cash, double change, int isSenior);
void generateReceiptFilename(char* filename, size_t sz, int customerID, const char* customer);
void listCustomerHistory(int customerID);
int checkLoyaltyEligibility(int purchaseCount, double totalSpent);
int isLoyaltyCodeValid(int customerID, unsigned long long code);
unsigned long long getLoyaltyCode(int customerID);
long getLoyaltyTimestamp(int customerID);
unsigned long long assignLoyaltyCode(int customerID, const char* customerName);
void updateLoyaltyRecord(int customerID, const char* name, unsigned long long newCode, long newTimestamp);

int getNextCustomerID(void);
Customer getCustomerDataByID(int idToFind);
int createNewCustomer(const char* name);
void recordCustomer(int customerID, const char* name, double amountSpent);
int compareCustomersByPurchase(const void* a, const void* b);
void printReceiptToPrinter(const char* filename);
void customerQuarry(int isAdmin);
void purchaseGas(ShiftSession* session);
void performZRead(ShiftSession* session);

Cashier* cashierLogin(void);

void ensureAdminFileExists(void);
int verifyAdmin(const char* user, const char* pass);
void adminMenu(const char* adminName);
void manageAdmins(void);
void viewLogsFiltered(int mode);
void editGasPrices(void);
void cashierQuarry(void);
void reprintLastReceipt(void);
void returnStock(const char* fuelName, double litersReturned);
int voidCustomerSale(int customerID, double amountToDeduct);
void voidTransaction(ShiftSession* session);
void exportToCSV(void);

/* =========================================================================
 * 5. HELPER FUNCTIONS
 * ========================================================================= */

void getPasswordInput(char* buffer, int maxLen) {
    int i = 0;
    int ch;
    while (1) {
        ch = _getch();

        if (ch == 13 || ch == 10) {
            buffer[i] = '\0';
            printf("\n");
            break;
        }
        else if (ch == 8 || ch == 127) {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
        }
        else if (i < maxLen - 1) {
            if (ch >= 32 && ch <= 126) {
                buffer[i] = (char)ch;
                i++;
                printf("*");
            }
        }
    }
}

void sanitizeInput(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (*src != '\"' && *src != '\'') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void pauseMessage(void) {
    printf("\n%s%sPress Enter to continue...%s", PAD, WHITE, RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

int getStrictInt(int min, int max) {
    char line[256];
    if (!fgets(line, sizeof(line), stdin)) {
        return -1;
    }
    line[strcspn(line, "\n")] = 0;

    if (strlen(line) == 0) {
        return -1;
    }
    if (isspace((unsigned char)line[0])) {
        return -1;
    }

    char* endptr;
    errno = 0;
    long val = strtol(line, &endptr, 10);

    if (line == endptr || *endptr != '\0') {
        return -1;
    }
    if (val < min || val > max) {
        return -1;
    }
    return (int)val;
}

double getStrictDouble(double min) {
    char line[256];
    if (!fgets(line, sizeof(line), stdin)) {
        return -1.0;
    }
    line[strcspn(line, "\n")] = 0;

    if (strlen(line) == 0 || isspace((unsigned char)line[0])) {
        return -1.0;
    }

    char* endptr;
    double val = strtod(line, &endptr);

    if (line == endptr || *endptr != '\0') {
        return -1.0;
    }
    if (val == -1.0) {
        return -1.0;
    }
    if (val < min) {
        return -1.0;
    }
    return val;
}

unsigned long long getStrictULL(void) {
    char line[256];
    if (!fgets(line, sizeof(line), stdin)) {
        return 0;
    }
    line[strcspn(line, "\n")] = 0;

    if (strlen(line) == 0) {
        return 0;
    }
    char* endptr;
    unsigned long long val = strtoull(line, &endptr, 10);

    if (line == endptr || *endptr != '\0') {
        return 0;
    }
    return val;
}

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

FILE* safeOpen(const char* path, const char* mode) {
    FILE* f = fopen(path, mode);
    return f;
}

void openImageInViewer(const char* filename) {
    char cmd[512];
    #ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", filename);
    #else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", filename);
    #endif
    system(cmd);
}

int hasNumbers(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (isdigit((unsigned char)str[i])) {
            return 1;
        }
    }
    return 0;
}

void showLoading() {
    printf("\n%s%sLoading%s", PAD, YELLOW, RESET);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        SLEEP(300);
    }
    printf("\n");
}

void pauseAndReturnToMenu(void) {
    printf("\n%s%sPress Enter to return to the menu...%s", PAD, WHITE, RESET);
    while (getchar() != '\n');
    showLoading();
    system(CLEAR_SCREEN);
}

/* =========================================================================
 * 6. EMAIL & CURL FUNCTIONS
 * ========================================================================= */

/* Buffer for dynamic email body */
char email_payload_buffer[2048];
size_t email_payload_len = 0;
size_t email_read_ptr = 0;

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t buffer_size = size * nmemb;
    size_t remaining = email_payload_len - email_read_ptr;

    if (remaining > 0) {
        size_t copy_size = (remaining < buffer_size) ? remaining : buffer_size;
        memcpy(ptr, email_payload_buffer + email_read_ptr, copy_size);
        email_read_ptr += copy_size;
        return copy_size;
    }
    return 0;
}

int sendEmailToSupplier(const char* customMessage) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *recipients = NULL;

    /* Construct payload dynamically */
    snprintf(email_payload_buffer, sizeof(email_payload_buffer),
        "To: %s\r\n"
        "From: %s\r\n"
        "Subject: Gasoline Restock Request\r\n"
        "\r\n"
        "Good day Supplier,\r\n"
        "\r\n"
        "%s\r\n"
        "\r\n"
        "Thank you.\r\n"
        "SAZAKE Gas Station\r\n",
        SUPPLIER_EMAIL, SENDER_EMAIL, customMessage);

    email_payload_len = strlen(email_payload_buffer);
    email_read_ptr = 0;

    curl = curl_easy_init();
    if (!curl) return 0;

    curl_easy_setopt(curl, CURLOPT_URL, SMTP_URL);
    curl_easy_setopt(curl, CURLOPT_USERNAME, EMAIL_USERNAME);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, EMAIL_PASSWORD);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, SENDER_EMAIL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    recipients = curl_slist_append(recipients, SUPPLIER_EMAIL);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    res = curl_easy_perform(curl);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

void savePricesToFile(void) {
    FILE* fp = fopen(PRICE_FILE, "w");
    if (!fp) return;

    for (int i = 0; i < fuelCount; ++i) {
        fprintf(fp, "%s=%.2f\n", fuels[i].name, fuels[i].price);
    }
    fclose(fp);
}

void loadPricesFromFile(void) {
    FILE* fp = fopen(PRICE_FILE, "r");
    if (!fp) {
        /* If file doesn't exist, prices remain 0 (which forces Admin set up) */
        savePricesToFile();
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        char* token = strtok(line, "=");
        if (!token) continue;
        char* priceStr = strtok(NULL, "=");
        if (!priceStr) continue;

        double price = strtod(priceStr, NULL);

        for (int i = 0; i < fuelCount; ++i) {
            #ifdef _WIN32
            if (_stricmp(fuels[i].name, token) == 0)
            #else
            if (strcasecmp(fuels[i].name, token) == 0)
            #endif
            {
                fuels[i].price = price;
            }
        }
    }
    fclose(fp);
}

int arePricesSet(void) {
    for(int i=0; i<fuelCount; i++) {
        if(fuels[i].price <= 0.01) {
            return 0;
        }
    }
    return 1;
}

/* =========================================================================
 * 7. LOGGING & STOCK MANAGEMENT
 * ========================================================================= */

void logStockTransaction(int id, const char* fuelName, double amount, const char* type) {
    FILE* fp = fopen(STOCK_LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    /* CHANGED: Removed the hardcoded '+' before %.2f so negatives show correctly */
    fprintf(fp, "[%02d-%02d-%04d %02d:%02d] %s | ID: %d (%s) | Amount: %.2f L\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min,
            type, id, fuelName, amount);

    fclose(fp);
}

void logCashierActivity(const char* cashierName, const char* action) {
    FILE* fp = fopen(CASHIER_LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] User: %s | Action: %s\n",
        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        cashierName, action);

    fclose(fp);
}

/* --- NEW: SEPARATE ADMIN LOGGER --- */
void logAdminActivity(const char* adminName, const char* action) {
    FILE* fp = fopen(ADMIN_LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] Admin: %s | Action: %s\n",
        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        adminName, action);

    fclose(fp);
}

void repairStockFile(void) {
    FILE *fp = fopen(STOCK_FILE, "w");
    if (fp) {
        for(int i=0; i < MAX_TYPES; i++) {
            fprintf(fp, "%d %.2f 0 0.00 0\n", 101 + i, DEFAULT_STOCK);
        }
        fclose(fp);
    }
}

int saveStocksToFile(GasolineStock stocks[], int count) {
    FILE *fp = fopen(STOCK_FILE, "w");
    if (!fp) {
        printf("%s[ERROR] Failed to open stock file for writing.%s\n", RED, RESET);
        return 0;
    }
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d %.2f %ld %.2f %ld\n",
            stocks[i].id,
            stocks[i].currentStock,
            (long)stocks[i].lastRestockTime,
            stocks[i].pendingAmount,
            (long)stocks[i].deliveryArrivalTime);
    }
    fclose(fp);
    return 1;
}

void processPendingDeliveries(GasolineStock stocks[], int count) {
    time_t now = time(NULL);
    int updated = 0;

    for (int i = 0; i < count; i++) {
        if (stocks[i].pendingAmount > 0) {
            if (now >= stocks[i].deliveryArrivalTime) {
                stocks[i].currentStock += stocks[i].pendingAmount;

                int arrayIndex = stocks[i].id - 101;
                const char* name = (arrayIndex >= 0 && arrayIndex < fuelCount) ? fuels[arrayIndex].name : "Unknown";

                logStockTransaction(stocks[i].id, name, stocks[i].pendingAmount, "DELIVERY-ARRIVED");

                stocks[i].pendingAmount = 0.0;
                stocks[i].deliveryArrivalTime = 0;
                updated = 1;
            }
        }
    }

    if (updated) {
        saveStocksToFile(stocks, count);
    }
}

int loadStocksFromFile(GasolineStock* stocks) {
    FILE *fp = fopen(STOCK_FILE, "r");

    if (!fp) {
        repairStockFile();
        fp = fopen(STOCK_FILE, "r");
        if (!fp) return 0;
    }

    int count = 0;
    while (count < MAX_TYPES) {
        long tempTime = 0;
        long deliveryTime = 0;
        double pending = 0.0;

        int readCount = fscanf(fp, "%d %lf %ld %lf %ld",
            &stocks[count].id,
            &stocks[count].currentStock,
            &tempTime,
            &pending,
            &deliveryTime);

        if (readCount == EOF) break;

        if (readCount >= 2) {
            if (stocks[count].id < 101 || stocks[count].id > 104) {
                count = 0;
                break;
            }

            stocks[count].lastRestockTime = (time_t)tempTime;

            if (readCount < 5) {
                stocks[count].pendingAmount = 0.0;
                stocks[count].deliveryArrivalTime = 0;
            } else {
                stocks[count].pendingAmount = pending;
                stocks[count].deliveryArrivalTime = (time_t)deliveryTime;
            }
            count++;
        } else {
            break;
        }
    }
    fclose(fp);

    if (count < MAX_TYPES) {
        printf("Detected corrupted/outdated stock file. Repairing...\n");
        repairStockFile();
        fp = fopen(STOCK_FILE, "r");
        count = 0;
        if (fp) {
            while (count < MAX_TYPES) {
                 fscanf(fp, "%d %lf %ld %lf %ld",
                    &stocks[count].id, &stocks[count].currentStock, &stocks[count].lastRestockTime,
                    &stocks[count].pendingAmount, &stocks[count].deliveryArrivalTime);
                 count++;
            }
            fclose(fp);
        }
    }

    processPendingDeliveries(stocks, count);
    return count;
}

void viewGasolineStock(void) {
    system(CLEAR_SCREEN);

    GasolineStock* stocks = (GasolineStock*)malloc(MAX_TYPES * sizeof(GasolineStock));
    if (!stocks) { printf("Memory Error.\n"); return; }

    int count = loadStocksFromFile(stocks);
    processPendingDeliveries(stocks, count);

    if (count == 0) {
        printf("%s%sNo gasoline stock records found.\n", PAD, RED, RESET);
        free(stocks);
        pauseAndReturnToMenu();
        return;
    }

    printf("\n%s%s======================== TANK LEVEL MONITOR ========================%s\n", PAD, MAGENTA, RESET);
    printf("%sMax Capacity per Tank: %.0f Liters\n", PAD, MAX_STOCK);

    time_t now = time(NULL);

    for (int i = 0; i < count; i++) {
        int arrayIndex = stocks[i].id - 101;
        const char* displayName = "Unknown";
        if (arrayIndex >= 0 && arrayIndex < fuelCount) {
            displayName = fuels[arrayIndex].name;
        }

        /* Calculate Percentage */
        double percent = (stocks[i].currentStock / MAX_STOCK) * 100.0;
        if(percent > 100.0) percent = 100.0; /* Cap visual at 100% */
        if(percent < 0.0) percent = 0.0;

        /* Determine Color based on Level */
        const char* barColor = GREEN;
        if (percent < 20.0) barColor = RED;
        else if (percent < 50.0) barColor = YELLOW;

        /* Draw the Tank Name */
        printf("\n%s%sTank %d: %s%s\n", PAD, CYAN, stocks[i].id, displayName, RESET);

        /* Draw the Progress Bar */
        int barWidth = 40; /* Total width of the bar in characters */
        int fillLen = (int)((percent / 100.0) * barWidth);

        printf("%s%s[", PAD, BOLD); /* Bracket */
        printf("%s", barColor);     /* Set Color */

        for (int b = 0; b < barWidth; b++) {
            if (b < fillLen) printf("|"); /* Fill character */
            else printf(" ");             /* Empty space */
        }

        printf("%s%s] %.1f%% (%.2f L)%s\n", RESET, BOLD, percent, stocks[i].currentStock, RESET);

        /* Status & Delivery Info */
        if (stocks[i].pendingAmount > 0) {
            double diff = difftime(stocks[i].deliveryArrivalTime, now);
            int min = (int)(diff / 60);
            if (min < 0) min = 0;
            printf("%s   %s>> DELIVERY INBOUND: +%.2f L (ETA: %d mins)%s\n", PAD, YELLOW, stocks[i].pendingAmount, min + 1, RESET);
        } else if (stocks[i].currentStock <= LOW_STOCK_THRESHOLD) {
            printf("%s   %s[CRITICAL WARNING] STOCK LEVEL CRITICAL! REORDER NOW.%s\n", PAD, RED, RESET);
        } else if (percent < 20.0) {
            printf("%s   %s[WARNING] Low Level.%s\n", PAD, YELLOW, RESET);
        } else {
            printf("%s   Status: Optimal\n", PAD);
        }
    }
    printf("\n%s%s=====================================================================%s\n", PAD, MAGENTA, RESET);
    free(stocks);
    pauseAndReturnToMenu();
}

/* Updated to accept adminName (can be NULL if auto-triggered, but mostly used by Admin now) */
void manualRestockMenu(const char* adminName) {
    system(CLEAR_SCREEN);
    GasolineStock* stocks = (GasolineStock*)malloc(MAX_TYPES * sizeof(GasolineStock));
    if (!stocks) return;

    int count = loadStocksFromFile(stocks);
    processPendingDeliveries(stocks, count);

    printf("\n%s%s===== MANUAL RESTOCK (Admin) =====%s\n", PAD, MAGENTA, RESET);
    for (int i = 0; i < count; i++) {
        int arrayIndex = stocks[i].id - 101;
        printf("%s%s%d.%s %s (Cur: %.2f L)", PAD, YELLOW, stocks[i].id, RESET,
               (arrayIndex >= 0 ? fuels[arrayIndex].name : "Unknown"),
               stocks[i].currentStock);

        if (stocks[i].pendingAmount > 0) {
            printf(" %s[PENDING: +%.2f]%s", YELLOW, stocks[i].pendingAmount, RESET);
        }
        printf("\n");
    }
    printf("%s0. Cancel\n", PAD);

    printf("\n%sEnter Gas ID to restock (101-104): ", PAD);
    int id = getStrictInt(0, 104);

    if (id <= 0) {
        if(id == -1) printf("%s%sInvalid Input. Please enter valid ID.\n", PAD, RED, RESET);
        free(stocks);
        if(id == -1) pauseMessage();
        return;
    }

    if (id < 101) {
        printf("%s%sInvalid Gas ID.\n", PAD, RED, RESET);
        free(stocks);
        pauseMessage();
        return;
    }

    int idx = -1;
    for(int i=0; i<count; i++) {
        if(stocks[i].id == id) { idx = i; break; }
    }

    if (idx == -1) { printf("%sError finding stock.\n", PAD); free(stocks); return; }

    if (stocks[idx].pendingAmount > 0) {
        printf("\n%s%s[ERROR] A delivery is already en route for this fuel.%s\n", PAD, RED, RESET);
        free(stocks);
        pauseMessage();
        return;
    }

    printf("%sCurrent Stock: %.2f L\n", PAD, stocks[idx].currentStock);
    printf("%sEnter amount to order (Max Capacity %.0f L): ", PAD, MAX_STOCK);

    double amount = getStrictDouble(0.1);
    if (amount == -1.0) { printf("%sInvalid amount.\n", PAD); free(stocks); pauseMessage(); return; }

    if (stocks[idx].currentStock + amount > MAX_STOCK) {
        printf("%s%sError: Capacity Exceeded.%s Max capacity is %.0f L.\n", PAD, RED, RESET, MAX_STOCK);
    } else {
        char customMsg[256];
        printf("%sEnter message for supplier (or Enter for default): ", PAD);
        fgets(customMsg, sizeof(customMsg), stdin);
        customMsg[strcspn(customMsg, "\n")] = 0;
        if(strlen(customMsg) == 0) strcpy(customMsg, "Standard automated restock request.");

        printf("%sContacting supplier...\n", PAD);

        if (sendEmailToSupplier(customMsg)) {
            stocks[idx].pendingAmount = amount;
            stocks[idx].deliveryArrivalTime = time(NULL) + DELIVERY_TIME_SECONDS;
            stocks[idx].lastRestockTime = time(NULL);

            saveStocksToFile(stocks, count);

            int arrayIndex = stocks[idx].id - 101;
            logStockTransaction(id, (arrayIndex >= 0 ? fuels[arrayIndex].name : "Unknown"), amount, "ADMIN-ORDER");
            if(adminName) {
                char actBuf[64];
                snprintf(actBuf, 64, "Restock Order: %s", fuels[arrayIndex].name);
                /* CHANGED: Use Admin Logger */
                logAdminActivity(adminName, actBuf);
            }

            printf("%s%sSuccess! Delivery truck dispatched.%s\n", PAD, GREEN, RESET);
            printf("%s%sEstimated arrival time: 30 Minutes.\n", PAD, YELLOW, RESET);
        } else {
            printf("%s%sFailed to contact supplier. Restock aborted.%s\n", PAD, RED, RESET);
        }
    }

    free(stocks);
    pauseAndReturnToMenu();
}

void stockMenu(void) {
    int choice;
    do {
        system(CLEAR_SCREEN);
        printf("\n%s%s===== Gasoline Stock Management (Viewer) =====%s\n", PAD, MAGENTA, RESET);
        printf("%s%s1.%s View Stocks (Levels & History)\n", PAD, CYAN, RESET);
        printf("%s%s2.%s Back to Main Menu\n", PAD, YELLOW, RESET);
        printf("%sEnter your choice: ", PAD);

        choice = getStrictInt(1, 2);

        if (choice == -1) {
            printf("%s%sInvalid input. Pick 1 or 2.\n", PAD, RED, RESET);
            SLEEP(500);
            continue;
        }

        switch (choice) {
            case 1: viewGasolineStock(); break;
            case 2: printf("%sExiting menu...\n", PAD); break;
        }
    } while (choice != 2);
}

int findStockIndexByID(GasolineStock stocks[], int count, int id) {
    for (int i = 0; i < count; i++) {
        if (stocks[i].id == id) {
            return i;
        }
    }
    return -1;
}

void deductStock(const char* fuelName, double litersSold) {
    int targetID = -1;
    for(int i=0; i<fuelCount; i++) {
        #ifdef _WIN32
        if (_stricmp(fuels[i].name, fuelName) == 0)
        #else
        if (strcasecmp(fuels[i].name, fuelName) == 0)
        #endif
        {
            targetID = 101 + i;
            break;
        }
    }
    if (targetID == -1) return;

    GasolineStock* stocks = (GasolineStock*)malloc(MAX_TYPES * sizeof(GasolineStock));
    if(!stocks) return;

    int count = loadStocksFromFile(stocks);
    processPendingDeliveries(stocks, count);

    int idx = findStockIndexByID(stocks, count, targetID);

    if (idx != -1) {
        stocks[idx].currentStock -= litersSold;
        if (stocks[idx].currentStock < 0) stocks[idx].currentStock = 0;
        saveStocksToFile(stocks, count);

        /* --- NEW: LOG THE SALE --- */
        /* We pass negative liters to indicate outflow */
        logStockTransaction(targetID, fuelName, -litersSold, "SALE");
    }

    free(stocks);
}

int checkStockForPurchase(const char* fuelName, double litersNeeded) {
    int targetID = -1;
    for(int i=0; i<fuelCount; i++) {
        #ifdef _WIN32
        if (_stricmp(fuels[i].name, fuelName) == 0)
        #else
        if (strcasecmp(fuels[i].name, fuelName) == 0)
        #endif
        {
            targetID = 101 + i;
            break;
        }
    }
    if (targetID == -1) return 0;

    GasolineStock* stocks = (GasolineStock*)malloc(MAX_TYPES * sizeof(GasolineStock));
    if(!stocks) return 0;

    int count = loadStocksFromFile(stocks);
    processPendingDeliveries(stocks, count);

    int idx = findStockIndexByID(stocks, count, targetID);

    int result = 1;

    if (idx != -1) {
        if (stocks[idx].currentStock < litersNeeded) {
            printf("\n%s%s[ALERT] Low Stocks for %s!%s\n", PAD, RED, fuelName, RESET);
            printf("%sCurrent: %.2f L | Needed: %.2f L\n", PAD, stocks[idx].currentStock, litersNeeded);

            if (stocks[idx].pendingAmount > 0) {
                 double diff = difftime(stocks[idx].deliveryArrivalTime, time(NULL));
                 int min = (int)(diff / 60) + 1;
                 printf("%s%sWait! A delivery truck is already en route.%s\n", PAD, YELLOW, RESET);
                 printf("%sArriving in approximately %d minutes.\n", PAD, min);
                 printf("%s%sCannot process transaction until fuel arrives.\n", PAD, RED, RESET);
                 result = 0;
            }
            else {
                printf("%sAutomatic Emergency Order logic...\n", PAD);
                char autoMsg[] = "EMERGENCY AUTO-ORDER: Low Stock during transaction.";
                if (sendEmailToSupplier(autoMsg)) {
                    printf("%s%sSupplier contacted...%s\n", PAD, GREEN, RESET);

                    stocks[idx].lastRestockTime = time(NULL);
                    stocks[idx].pendingAmount = MAX_STOCK - stocks[idx].currentStock;
                    stocks[idx].deliveryArrivalTime = time(NULL) + DELIVERY_TIME_SECONDS;

                    saveStocksToFile(stocks, count);

                    logStockTransaction(targetID, fuelName, stocks[idx].pendingAmount, "AUTO-ORDER");

                    printf("%s%sEmergency Delivery Scheduled. ETA: 30 Minutes.%s\n", PAD, YELLOW, RESET);
                    printf("%sPlease ask customer to wait or return later.\n", PAD);

                    result = 0;
                } else {
                    printf("%s%sError: Could not contact supplier. Transaction Aborted.%s\n", PAD, RED, RESET);
                    result = 0;
                }
            }
        }
    }

    free(stocks);
    return result;
}

/* =========================================================================
 * 8. CUSTOMER, LOYALTY & UI FUNCTIONS
 * ========================================================================= */

int checkLoyaltyEligibility(int purchaseCount, double totalSpent) {
    return (purchaseCount >= LOYALTY_MIN_PURCHASES && totalSpent >= LOYALTY_MIN_SPENT);
}

int isLoyaltyCodeValid(int customerID, unsigned long long code) {
    FILE* fp = safeOpen(LOYALTY_FILE, "r");
    if (!fp) return 0;
    int storedID;
    char storedName[256];
    unsigned long long storedCode;
    long assignmentTimestamp;

    int foundValid = 0;

    while (fscanf(fp, "%d \"%255[^\"]\" %llu %ld", &storedID, storedName, &storedCode, &assignmentTimestamp) == 4) {
        if (storedID == customerID && storedCode == code) {
            time_t now = time(NULL);
            time_t expiryTime = assignmentTimestamp + LOYALTY_EXPIRY_SECONDS;

            if (now > expiryTime) {
                foundValid = -1; /* Expired */
            } else {
                foundValid = 1; /* Valid */
            }
        }
    }
    fclose(fp);
    return foundValid;
}

unsigned long long getLoyaltyCode(int customerID) {
    FILE* fp = safeOpen(LOYALTY_FILE, "r");
    if (!fp) return 0;
    int storedID;
    char storedName[256];
    unsigned long long storedCode;
    long timestamp;

    unsigned long long latestCode = 0;

    while (fscanf(fp, "%d \"%255[^\"]\" %llu %ld", &storedID, storedName, &storedCode, &timestamp) == 4) {
        if (storedID == customerID) {
            latestCode = storedCode;
        }
    }
    fclose(fp);
    return latestCode;
}

long getLoyaltyTimestamp(int customerID) {
    FILE* fp = safeOpen(LOYALTY_FILE, "r");
    if (!fp) return 0;
    int storedID;
    char storedName[256];
    unsigned long long storedCode;
    long timestamp;

    long latestTimestamp = 0;

    while (fscanf(fp, "%d \"%255[^\"]\" %llu %ld", &storedID, storedName, &storedCode, &timestamp) == 4) {
        if (storedID == customerID) {
            latestTimestamp = timestamp;
        }
    }
    fclose(fp);
    return latestTimestamp;
}

unsigned long long assignLoyaltyCode(int customerID, const char* customerName) {
    unsigned long long r1 = ((unsigned long long)rand() << 32) ^ rand() ^ ((unsigned long long)time(NULL));
    unsigned long long code = (r1 & 0xFFFFFFFFFFFFULL) % 100000000000000ULL;
    FILE* fp = safeOpen(LOYALTY_FILE, "a");
    if (fp) {
        fprintf(fp, "%d \"%s\" %llu %ld\n", customerID, customerName, code, (long)time(NULL));
        fclose(fp);
    }
    return code;
}

/* Helper to rewrite loyalty file when Admin edits it */
void updateLoyaltyRecord(int customerID, const char* name, unsigned long long newCode, long newTimestamp) {
    FILE* fp = fopen(LOYALTY_FILE, "a");
    if(fp) {
        fprintf(fp, "%d \"%s\" %llu %ld\n", customerID, name, newCode, newTimestamp);
        fclose(fp);
    }
}

int getNextCustomerID(void) {
    int nextID = 1000;
    FILE* fp = fopen(CUSTOMER_ID_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d", &nextID) != 1) nextID = 1000;
        fclose(fp);
    }
    else {
        FILE* wf = fopen(CUSTOMER_ID_FILE, "w");
        if (wf) {
            fprintf(wf, "%d\n", nextID + 1);
            fclose(wf);
            return nextID;
        }
    }
    int toWrite = nextID + 1;
    if (toWrite > 9999) toWrite = 1000;
    fp = fopen(CUSTOMER_ID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d\n", toWrite);
        fclose(fp);
    }
    return nextID;
}

Customer getCustomerDataByID(int idToFind) {
    Customer cust = { 0 };
    FILE* fp = safeOpen(CUSTOMER_FILE, "r");
    if (!fp) return cust;
    int id, count;
    char storedName[256];
    double spent;
    while (fscanf(fp, "%d \"%255[^\"]\" %d %lf", &id, storedName, &count, &spent) == 4) {
        if (id == idToFind) {
            cust.id = id;
            strncpy(cust.name, storedName, sizeof(cust.name) - 1);
            cust.purchaseCount = count;
            cust.totalSpent = spent;
            break;
        }
    }
    fclose(fp);
    return cust;
}

int createNewCustomer(const char* name) {
    int newID = getNextCustomerID();
    FILE* fp = fopen(CUSTOMER_FILE, "a");
    if (!fp) {
        fprintf(stderr, "Could not open %s to create customer.\n", CUSTOMER_FILE);
        return 0;
    }
    fprintf(fp, "%d \"%s\" %d %.2f\n", newID, name, 0, 0.0);
    fclose(fp);
    return newID;
}

void recordCustomer(int customerID, const char* name, double amountSpent) {
    Customer* customers = (Customer*)malloc(MAX_CUSTOMERS * sizeof(Customer));
    if (!customers) {
        printf("%s%s[CRITICAL ERROR] Not enough memory to save customer record.\n", PAD, RED, RESET);
        return;
    }

    int customerCount = 0;
    int found = 0;

    FILE* fp = fopen(CUSTOMER_FILE, "r");
    if (fp) {
        while (customerCount < MAX_CUSTOMERS &&
            fscanf(fp, "%d \"%255[^\"]\" %d %lf",
                &customers[customerCount].id,
                customers[customerCount].name,
                &customers[customerCount].purchaseCount,
                &customers[customerCount].totalSpent) == 4) {
            customerCount++;
        }
        fclose(fp);
    }

    for (int i = 0; i < customerCount; i++) {
        if (customers[i].id == customerID) {
            customers[i].purchaseCount++;
            customers[i].totalSpent += amountSpent;
            found = 1;
            break;
        }
    }

    if (!found && customerCount < MAX_CUSTOMERS) {
        customers[customerCount].id = customerID;
        strncpy(customers[customerCount].name, name, sizeof(customers[customerCount].name) - 1);
        customers[customerCount].purchaseCount = 1;
        customers[customerCount].totalSpent = amountSpent;
        customerCount++;
    }

    fp = fopen(CUSTOMER_FILE, "w");
    if (fp) {
        for (int i = 0; i < customerCount; i++) {
            fprintf(fp, "%d \"%s\" %d %.2f\n",
                customers[i].id,
                customers[i].name,
                customers[i].purchaseCount,
                customers[i].totalSpent);
        }
        fclose(fp);
    }

    free(customers);
}

int compareCustomersByPurchase(const void* a, const void* b) {
    const Customer* custA = (const Customer*)a;
    const Customer* custB = (const Customer*)b;
    return custB->purchaseCount - custA->purchaseCount;
}

void generateBarcodeImage(const char* data, const char* filename) {
    struct zint_symbol* symbol = ZBarcode_Create();
    if (!symbol) {
        fprintf(stderr, "Failed to create barcode symbol\n");
        return;
    }
    symbol->symbology = BARCODE_CODE128;
    strcpy(symbol->outfile, filename);
    int ret = ZBarcode_Encode_and_Print(symbol, (unsigned char*)data, 0, 0);
    if (ret != 0) {
        fprintf(stderr, "Barcode error: %s\n", symbol->errtxt);
    } else {
        printf("Loyalty barcode image saved to %s\n", filename);
    }
    ZBarcode_Delete(symbol);
}

void printReceiptFormatted(FILE* out, const char* cashierName, int customerID, const char* customer,
    Item* items, int itemCount,
    double subtotal, double tax, double discount, double grandTotal,
    double cash, double change, int isSenior) {
    char plBuf[64], subBuf[64];

    /* Note: We refrain from using ANSI colors inside the receipt file itself to avoid garbled text in Notepad */
    fprintf(out, "=========================================================\n");
    fprintf(out, "                       SAZAKE GAS STATION\n");
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t) {
        fprintf(out, "Date: %02d-%02d-%04d    Time: %02d:%02d:%02d\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec);
    }
    fprintf(out, "---------------------------------------------------------\n");
    fprintf(out, "Cashier: %s    Customer ID: %04d\n", cashierName ? cashierName : "Unknown", customerID);
    fprintf(out, "Customer: %s\n", customer);
    fprintf(out, "---------------------------------------------------------\n");
    fprintf(out, "%-*s %*s %*s %*s\n",
        FUEL_W, "Fuel Type",
        LITERS_W, "Liters",
        PL_W, "P/Liter",
        SUB_W, "Subtotal");
    fprintf(out, "---------------------------------------------------------\n");

    for (int i = 0; i < itemCount; ++i) {
        snprintf(plBuf, sizeof(plBuf), "PHP %.2f", items[i].pricePerLiter);
        snprintf(subBuf, sizeof(subBuf), "PHP %.2f", items[i].subtotal);
        fprintf(out, "%-*s %*.*f %*s %*s\n",
            FUEL_W, fuels[items[i].fuelIndex].name,
            LITERS_W, 2, items[i].liters,
            PL_W, plBuf,
            SUB_W, subBuf);
    }
    fprintf(out, "---------------------------------------------------------\n");

    snprintf(subBuf, sizeof(subBuf), "PHP %.2f", subtotal);
    fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "SUBTOTAL:", SUB_W, subBuf);

    if (isSenior) {
        fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "TAX (0%):", SUB_W, "VAT EXEMPT");
    } else {
        snprintf(subBuf, sizeof(subBuf), "PHP %.2f", tax);
        fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "TAX (12%):", SUB_W, subBuf);
    }

    if (discount > 0.0) {
        snprintf(subBuf, sizeof(subBuf), "-PHP %.2f", discount);
        if (isSenior) {
            fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "SC/PWD DISC:", SUB_W, subBuf);
        } else {
            fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "LOYALTY (10%):", SUB_W, subBuf);
        }
    }
    snprintf(subBuf, sizeof(subBuf), "PHP %.2f", grandTotal);
    fprintf(out, "---------------------------------------------------------\n");
    fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "GRAND TOTAL:", SUB_W, subBuf);

    snprintf(subBuf, sizeof(subBuf), "PHP %.2f", cash);
    fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "CASH:", SUB_W, subBuf);
    snprintf(subBuf, sizeof(subBuf), "PHP %.2f", change);
    fprintf(out, "%-*s %*s\n", FUEL_W + 1 + LITERS_W + 1 + PL_W - 1, "CHANGE:", SUB_W, subBuf);
    fprintf(out, "---------------------------------------------------------\n");
    fprintf(out, "        THANK YOU FOR CHOOSING SAZAKE GAS STATION!\n");
    fprintf(out, "---------------------------------------------------------\n");
}

void generateReceiptFilename(char* filename, size_t sz, int customerID, const char* customer) {
#ifdef _WIN32
    system(RECEIPTS_DIR_CMD_WIN);
#else
    system(RECEIPTS_DIR_CMD_UNIX);
#endif
    char safeName[64] = { 0 };
    int j = 0;
    for (size_t i = 0; i < strlen(customer) && j < (int)sizeof(safeName) - 1; ++i) {
        char c = customer[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            safeName[j++] = c;
        }
        else {
            safeName[j++] = '_';
        }
    }
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t) {
        snprintf(filename, sz, BASE_RECEIPT_PATH "receipt_%04d_%s_%04d%02d%02d_%02d%02d%02d.txt",
            customerID,
            safeName[0] ? safeName : "guest",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    }
    else {
        snprintf(filename, sz, BASE_RECEIPT_PATH "receipt_%04d_%s.txt", customerID, safeName[0] ? safeName : "guest");
    }
}

void listCustomerHistory(int customerID) {
    printf("\n%s--- Transaction History for ID %04d ---\n", TABLE_PAD, customerID);

    char searchPattern[128];
    snprintf(searchPattern, sizeof(searchPattern), BASE_RECEIPT_PATH "receipt_%04d_*.txt", customerID);

#ifdef _WIN32
    struct _finddata_t file_info;
    intptr_t handle = _findfirst(searchPattern, &file_info);

    if (handle == -1) {
        printf("%sNo receipt history found.\n", TABLE_PAD);
    } else {
        printf("%s%-30s\n", TABLE_PAD, "Receipt Filename (Has Date)");
        printf("%s------------------------------\n", TABLE_PAD);
        do {
            printf("%s%s\n", TABLE_PAD, file_info.name);
        } while (_findnext(handle, &file_info) == 0);
        _findclose(handle);
    }
#else
    printf("(History view supported on Windows for this version)\n");
#endif
    printf("%s------------------------------\n", TABLE_PAD);
}

void customerQuarry(int isAdmin) {
    clearScreen();

    Customer* customers = (Customer*)malloc(MAX_CUSTOMERS * sizeof(Customer));
    if (!customers) {
        printf("CRITICAL ERROR: Memory allocation failed.\n");
        pauseMessage();
        return;
    }

    int customerCount = 0;

    FILE* fp = safeOpen(CUSTOMER_FILE, "r");
    if (!fp) {
        printf("%sNo customer records yet.\n", PAD);
        free(customers);
        pauseMessage();
        return;
    }

    /* 1. Load Data */
    while (customerCount < MAX_CUSTOMERS &&
        fscanf(fp, "%d \"%255[^\"]\" %d %lf",
            &customers[customerCount].id,
            customers[customerCount].name,
            &customers[customerCount].purchaseCount,
            &customers[customerCount].totalSpent) == 4) {
        customers[customerCount].loyaltyCode = getLoyaltyCode(customers[customerCount].id);
        customerCount++;
    }
    fclose(fp);

    if (customerCount == 0) {
        printf("%sNo customer records yet.\n", PAD);
        free(customers);
        pauseMessage();
        return;
    }

    /* Sort by purchase count (VIPs at top) */
    qsort(customers, customerCount, sizeof(Customer), compareCustomersByPurchase);

    /* 2. SEARCH MENU */
    char searchStr[128] = "";
    int showAll = 0;

    while(1) {
        clearScreen();
        printf("\n%s%s===== CUSTOMER DATABASE =====%s\n", PAD, MAGENTA, RESET);
        printf("%s1. Show Top 20 Customers\n", PAD);
        printf("%s2. Search by Name\n", PAD);
        printf("%s0. Exit\n", PAD);
        printf("%sChoice: ", PAD);

        int mode = getStrictInt(0, 2);

        if (mode == 0) { free(customers); return; }

        if (mode == 2) {
            printf("%sEnter Name (or part of name): ", PAD);
            if(fgets(searchStr, sizeof(searchStr), stdin)) {
                searchStr[strcspn(searchStr, "\n")] = 0;
            }
            showAll = 0;
        } else {
            showAll = 1;
        }

        /* 3. DISPLAY RESULTS TABLE */
        printf("\n%s%s%-5s | %-25s | %-10s | %-15s | %-15s%s\n", TABLE_PAD, CYAN, "ID", "Customer Name", "Purchases", "Total Spent", "Loyalty Code", RESET);
        printf("%s------------------------------------------------------------------------------------------\n", TABLE_PAD);

        int matches = 0;
        for (int i = 0; i < customerCount; i++) {
            /* Filter Logic: Show if mode is "Show All" (limit 20) OR if name contains search string */
            int isMatch = 0;

            if (showAll) {
                if (i < 20) isMatch = 1; /* Only show top 20 */
            } else {
                /* Case-insensitive substring search simulation */
                char nameCopy[256];
                char searchCopy[128];
                strcpy(nameCopy, customers[i].name);
                strcpy(searchCopy, searchStr);

                /* Convert to lowercase for comparison */
                for(int c=0; nameCopy[c]; c++) nameCopy[c] = tolower(nameCopy[c]);
                for(int c=0; searchCopy[c]; c++) searchCopy[c] = tolower(searchCopy[c]);

                if (strstr(nameCopy, searchCopy) != NULL) isMatch = 1;
            }

            if (isMatch) {
                printf("%s%-5d | %-25s | %-10d | PHP %-12.2f | %-15llu\n", TABLE_PAD,
                    customers[i].id,
                    customers[i].name,
                    customers[i].purchaseCount,
                    customers[i].totalSpent,
                    customers[i].loyaltyCode ? customers[i].loyaltyCode : 0);
                matches++;
            }
        }
        printf("%s==========================================================================================\n", TABLE_PAD);
        if(matches == 0) printf("%sNo matches found for '%s'.\n", TABLE_PAD, searchStr);

        /* 4. SELECT ACTION */
        printf("\n%sEnter ID to view details (0 to search again): ", PAD);
        int searchID = getStrictInt(0, 99999);

        if (searchID == 0) continue; /* Loop back to search menu */

        /* ... Existing Logic for viewing/editing specific ID ... */
        int found = 0;
        for (int i = 0; i < customerCount; i++) {
            if (customers[i].id == searchID) {
                /* --- PASTE YOUR EXISTING VIEW/EDIT LOGIC HERE --- */
                /* (I have included the standard logic below for copy-paste ease) */

                printf("\n%s%s--- Details for Customer ID: %04d ---%s\n", TABLE_PAD, BOLD, searchID, RESET);
                printf("%sName:             %s\n", TABLE_PAD, customers[i].name);
                printf("%sPurchases:        %d\n", TABLE_PAD, customers[i].purchaseCount);
                printf("%sTotal Spent:      PHP %.2f\n", TABLE_PAD, customers[i].totalSpent);

                unsigned long long lCode = getLoyaltyCode(searchID);
                int canEdit = (lCode != 0 || checkLoyaltyEligibility(customers[i].purchaseCount, customers[i].totalSpent));

                if (lCode != 0) {
                    long ts = getLoyaltyTimestamp(searchID);
                    time_t now = time(NULL);
                    time_t expiry = ts + LOYALTY_EXPIRY_SECONDS;
                    printf("%sLoyalty Code:     %llu\n", TABLE_PAD, lCode);
                    if (now < expiry) printf("%sLoyalty Status:  %sACTIVE%s\n", TABLE_PAD, GREEN, RESET);
                    else printf("%sLoyalty Status:  %sEXPIRED%s\n", TABLE_PAD, RED, RESET);
                } else {
                    printf("%sLoyalty Status:  N/A\n", TABLE_PAD);
                }

                if(isAdmin) {
                    printf("\n%s%s[ADMIN OPTIONS]%s\n", TABLE_PAD, MAGENTA, RESET);
                    printf("%s1. View History\n", TABLE_PAD);
                    if (canEdit) printf("%s2. Edit Loyalty (Code & Duration)\n", TABLE_PAD);
                    else printf("%s(2. Edit Loyalty - LOCKED: Not Eligible)\n", TABLE_PAD);
                    printf("%s0. Back\n", TABLE_PAD);

                    printf("%sChoice: ", TABLE_PAD);
                    int admChoice = getStrictInt(0, 2);

                    if(admChoice == 1) listCustomerHistory(customers[i].id);
                    else if(admChoice == 2 && canEdit) {
                        printf("%sEnter NEW Loyalty Code (Numeric only): ", TABLE_PAD);
                        unsigned long long newCode = getStrictULL();
                        if(newCode > 0) {
                            updateLoyaltyRecord(customers[i].id, customers[i].name, newCode, (long)time(NULL));
                            printf("%sLoyalty Updated.\n", TABLE_PAD);
                            // Generate barcode logic...
                            char codeString[32];
                            snprintf(codeString, sizeof(codeString), "%llu", newCode);
                            char barcodeFilename[256];
                            snprintf(barcodeFilename, sizeof(barcodeFilename), BASE_BARCODE_PATH "loyalty_barcode_%04d.png", customers[i].id);
                            generateBarcodeImage(codeString, barcodeFilename);
                            openImageInViewer(barcodeFilename);
                        }
                    }
                } else {
                    listCustomerHistory(customers[i].id);
                }

                pauseMessage();
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("%sID %04d not found in this list.\n", PAD, searchID);
            pauseMessage();
        }
    }
}

void printReceiptToPrinter(const char* filename) {
#ifdef _WIN32
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "notepad /p \"%s\"", filename);
    int result = system(cmd);
    if (result != 0) fprintf(stderr, "Failed to send receipt to printer.\n");
#else
    (void)filename;
#endif
}

/* =========================================================================
 * 9. MAIN TRANSACTION LOGIC (With Shift Session)
 * ========================================================================= */

void purchaseGas(ShiftSession* session) {
    loadPricesFromFile();
    if(!arePricesSet()) {
        printf("\n%s%s[ERROR] Gas Prices are not set!%s\n", PAD, RED, RESET);
        printf("%sPlease ask the Admin to set prices.\n", PAD);
        pauseMessage();
        return;
    }

    char customerName[256] = { 0 };
    int customerID = 0;
    int attempts = 0;

transaction_reset:
    customerName[0] = '\0';
    customerID = 0;
    attempts = 0;

step_customer_type:
    clearScreen();
    int custType = 0;

    printf("\n%s%s===== SAZAKE GAS STATION - PURCHASE =====%s\n", PAD, MAGENTA, RESET);
    while (1) {
        if (attempts >= MAX_ATTEMPTS) { printf("%sToo many attempts. Returning to menu.\n", PAD); pauseMessage(); return; }

        printf("\n%sIs this a New or Existing customer?\n", PAD);
        printf("%s%s1.%s New Customer\n", PAD, GREEN, RESET);
        printf("%s%s2.%s Existing Customer\n", PAD, YELLOW, RESET);
        printf("%s%s0.%s Back to Main Menu\n", PAD, RED, RESET);
        printf("%sChoice: ", PAD);

        custType = getStrictInt(0, 2);

        if (custType == -1) {
            attempts++;
            printf("%sInvalid input. Please enter 1, 2, or 0. (%d left)\n", PAD, MAX_ATTEMPTS - attempts);
            pauseMessage();
            continue;
        }

        if (custType == 0) return;
        break;
    }

step_customer_details:
    if (custType == 1) {
        printf("\n%s(Enter 0 to Undo, -1 to Reset)\n", PAD);

        while(1) {
            printf("%sEnter new customer NAME: ", PAD);
            if (!fgets(customerName, sizeof(customerName), stdin)) return;
            customerName[strcspn(customerName, "\n")] = '\0';

            if (strcmp(customerName, "0") == 0) goto step_customer_type;
            if (strcmp(customerName, "-1") == 0) goto transaction_reset;

            if (strlen(customerName) == 0) {
                printf("%sName cannot be empty.\n", PAD);
                continue;
            }

            if (hasNumbers(customerName)) {
                printf("%sInvalid Input: Name cannot contain numbers. Please try again.\n", PAD);
                continue;
            }

            sanitizeInput(customerName);

            break;
        }

        int assigned = createNewCustomer(customerName);
        if (assigned == 0) return;
        customerID = assigned;
        printf("%sNew customer created: %s (ID: %04d)\n", PAD, customerName, customerID);
        pauseMessage();
    }
    else {
        attempts = 0;
        int idFound = 0;
        while(1) {
            if (attempts >= MAX_ATTEMPTS) { printf("%sLimit reached. Resetting.\n", PAD); pauseMessage(); goto transaction_reset; }

            printf("\n%s(Enter 0 to Undo, -1 to Reset)\n", PAD);
            printf("%sEnter existing CUSTOMER ID (4 digits): ", PAD);

            int tempID = getStrictInt(-1, 99999);

            if (tempID == -1) goto transaction_reset;
            if (tempID == 0) goto step_customer_type;

            if (tempID < 1000 && tempID != 0 && tempID != -1) {
                 printf("%sInvalid ID format. IDs start at 1000.\n", PAD);
                 attempts++;
                 continue;
            }

            Customer c = getCustomerDataByID(tempID);
            if (c.id != 0) {
                customerID = c.id;
                strncpy(customerName, c.name, sizeof(customerName) - 1);
                printf("%sLoaded customer: %s\n", PAD, c.name);
                pauseMessage();
                idFound = 1;
                break;
            } else {
                attempts++;
                printf("%sID not found. (%d left)\n", PAD, MAX_ATTEMPTS - attempts);
                pauseMessage();
            }
        }
        if (!idFound) return;
    }

step_purchase_mode:
    attempts = 0;
    int purchaseMode = 0;
    while(1) {
        if (attempts >= MAX_ATTEMPTS) { printf("%sLimit reached. Resetting.\n", PAD); pauseMessage(); goto transaction_reset; }

        clearScreen();
        printf("\n%sCustomer: %s%s%s (ID: %04d)\n", PAD, WHITE, customerName, RESET, customerID);
        printf("\n%sSelect Purchase Mode:\n", PAD);
        printf("%s%s1.%s Buy via Cash Amount (Auto-calc Liters)\n", PAD, GREEN, RESET);
        printf("%s%s2.%s Buy via Liters (Standard)\n", PAD, YELLOW, RESET);
        /* --- NEW MENU OPTION --- */
        printf("%s%s3.%s Renew Loyalty Code\n", PAD, CYAN, RESET);
        printf("%s%s0.%s Undo (Back to Customer Selection)\n", PAD, RED, RESET);
        printf("%sChoice: ", PAD);

        int input = getStrictInt(-1, 3);

        if (input == -1) goto transaction_reset;
        if (input < 0) {
            attempts++;
            printf("%sInvalid input. (%d left)\n", PAD, MAX_ATTEMPTS - attempts);
            pauseMessage();
            continue;
        }

        if (input == 0) goto step_customer_details;

        /* --- OPTION 3: RENEWAL LOGIC --- */
        if (input == 3) {
            unsigned long long currentCode = getLoyaltyCode(customerID);

            if (currentCode != 0) {
                /* Code Exists - Check if Valid */
                int status = isLoyaltyCodeValid(customerID, currentCode);

                if (status == 1) {
                    /* ACTIVE CODE */
                    long ts = getLoyaltyTimestamp(customerID);
                    time_t expiry = ts + LOYALTY_EXPIRY_SECONDS;
                    struct tm* t = localtime(&expiry);
                    char dateBuf[64];
                    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", t);

                    printf("\n%s%sYour Loyalty Code is still ACTIVE!%s\n", PAD, GREEN, RESET);
                    printf("%sExpires on: %s\n", PAD, dateBuf);
                }
                else {
                    /* EXPIRED CODE */
                    printf("\n%s%sYour Loyalty Code has EXPIRED.%s\n", PAD, RED, RESET);

                    Customer custData = getCustomerDataByID(customerID);
                    if (checkLoyaltyEligibility(custData.purchaseCount, custData.totalSpent)) {
                        printf("%sYou are eligible for renewal!\n", PAD);
                        printf("%sRenew now? (y/n): ", PAD);
                        char resp[256];
                        if (fgets(resp, sizeof(resp), stdin)) {
                            if (tolower(resp[0]) == 'y') {
                                unsigned long long newCode = assignLoyaltyCode(customerID, customerName);
                                printf("\n%s%sRENEWAL SUCCESSFUL!%s\n", PAD, GREEN, RESET);
                                printf("%sNew Code: %llu\n", PAD, newCode);
                                logCashierActivity(session->cashierName, "Renewed Loyalty Code");

                                printf("%sPrint physical card? (y/n): ", PAD);
                                if (fgets(resp, sizeof(resp), stdin) && tolower(resp[0]) == 'y') {
                                    #ifdef _WIN32
                                    system(BARCODES_DIR_CMD_WIN);
                                    #else
                                    system(BARCODES_DIR_CMD_UNIX);
                                    #endif
                                    char codeString[32];
                                    snprintf(codeString, sizeof(codeString), "%llu", newCode);
                                    char barcodeFilename[256];
                                    snprintf(barcodeFilename, sizeof(barcodeFilename), BASE_BARCODE_PATH "loyalty_barcode_%04d.png", customerID);
                                    generateBarcodeImage(codeString, barcodeFilename);
                                    openImageInViewer(barcodeFilename);
                                }
                            }
                        }
                    } else {
                        printf("%sYou are not eligible for renewal yet.\n", PAD);
                        printf("%sNeed: %d purchases & %.2f spent.\n", PAD, LOYALTY_MIN_PURCHASES, LOYALTY_MIN_SPENT);
                    }
                }
            } else {
                /* NO CODE EXISTS */
                printf("\n%sYou do not have a Loyalty Code.\n", PAD);
                Customer custData = getCustomerDataByID(customerID);
                if (checkLoyaltyEligibility(custData.purchaseCount, custData.totalSpent)) {
                    printf("%sBut you are eligible for one!\n", PAD);
                    printf("%sCreate now? (y/n): ", PAD);
                      char resp[256];
                        if (fgets(resp, sizeof(resp), stdin)) {
                            if (tolower(resp[0]) == 'y') {
                                unsigned long long newCode = assignLoyaltyCode(customerID, customerName);
                                printf("\n%s%sCREATED SUCCESSFULY!%s\n", PAD, GREEN, RESET);
                                printf("%sNew Code: %llu\n", PAD, newCode);
                                logCashierActivity(session->cashierName, "Created Loyalty Code");

                                printf("%sPrint physical card? (y/n): ", PAD);
                                if (fgets(resp, sizeof(resp), stdin) && tolower(resp[0]) == 'y') {
                                    #ifdef _WIN32
                                    system(BARCODES_DIR_CMD_WIN);
                                    #else
                                    system(BARCODES_DIR_CMD_UNIX);
                                    #endif
                                    char codeString[32];
                                    snprintf(codeString, sizeof(codeString), "%llu", newCode);
                                    char barcodeFilename[256];
                                    snprintf(barcodeFilename, sizeof(barcodeFilename), BASE_BARCODE_PATH "loyalty_barcode_%04d.png", customerID);
                                    generateBarcodeImage(codeString, barcodeFilename);
                                    openImageInViewer(barcodeFilename);
                                }
                            }
                        }
                } else {
                      printf("%sYou are not eligible yet.\n", PAD);
                }
            }
            pauseMessage();
            continue; /* Loop back to mode selection */
        }

        purchaseMode = input;
        break;
    }

    Item items[MAX_ITEMS];
    int itemCount = 0;
    double cashAvailable = 0.0;

    if (purchaseMode == 1) {
step_cash_entry:
        attempts = 0;
        while(1) {
            if (attempts >= MAX_ATTEMPTS) goto transaction_reset;
            printf("\n%s(Enter 0 to Undo, -1 to Reset)\n", PAD);
            printf("%sEnter total cash available (PHP): ", PAD);

            cashAvailable = getStrictDouble(0.0);

            if (cashAvailable == -1.0) goto transaction_reset;
            if (cashAvailable == 0.0) goto step_purchase_mode;

            if (cashAvailable < 0.0) {
                attempts++;
                printf("%sInvalid amount. Numeric only. (%d left)\n", PAD, MAX_ATTEMPTS - attempts);
                pauseMessage();
                continue;
            }
            break;
        }

        unsigned long long loyaltyCode = getLoyaltyCode(customerID);
        printf("\n%s===== Customer Info =====\n", PAD);
        printf("%sCustomer: %s (ID: %04d)\n", PAD, customerName, customerID);
        if (loyaltyCode) {
            long ts = getLoyaltyTimestamp(customerID);
            time_t now = time(NULL);
            time_t expiry = ts + LOYALTY_EXPIRY_SECONDS;
            if (now > expiry) printf("%sLoyalty: %llu (Expired)\n", PAD, loyaltyCode);
            else printf("%sLoyalty: %llu (%ld days left)\n", PAD, loyaltyCode, (long)difftime(expiry, now) / (24 * 60 * 60));
        } else {
            printf("%sLoyalty: N/A\n", PAD);
        }

        attempts = 0;
        int choice;
        while(1) {
            if (attempts >= MAX_ATTEMPTS) goto transaction_reset;

            printf("\n%s===== Select Fuel for PHP %.2f =====\n", PAD, cashAvailable);
            for (int i = 0; i < fuelCount; ++i) {
                printf("%s%d. %s - %.2f per liter\n", PAD, i + 1, fuels[i].name, fuels[i].price);
            }
            printf("%s0. Undo (Change Cash Amount)\n", PAD);
            printf("%s-1. Reset Transaction\n", PAD);
            printf("%sSelect fuel number: ", PAD);

            choice = getStrictInt(-1, fuelCount);

            if (choice == -1) goto transaction_reset;
            if (choice == 0) goto step_cash_entry;

            if (choice < 1) {
                attempts++;
                printf("%sInvalid selection.\n", PAD);
                continue;
            }
            break;
        }

        double subtotalFromCash = cashAvailable / (1.0 + TAX_RATE);
        double litersCalculated = subtotalFromCash / fuels[choice - 1].price;

        /* This check needs to happen even in cash mode */
        if (!checkStockForPurchase(fuels[choice - 1].name, litersCalculated)) {
            pauseMessage();
            goto step_cash_entry; /* Let them try another fuel or wait */
        }

        Item it;
        it.fuelIndex = choice - 1;
        it.liters = litersCalculated;
        it.pricePerLiter = fuels[it.fuelIndex].price;
        it.subtotal = it.liters * it.pricePerLiter;
        items[itemCount++] = it;

    } else {
step_fuel_loop:
        while (1) {
            clearScreen();
            unsigned long long loyaltyCode = getLoyaltyCode(customerID);
            printf("\n%s===== Fuel Selection (By Liters) =====\n", PAD);
            printf("%sCustomer: %s (ID: %04d)\n", PAD, customerName, customerID);

            printf("\n%sSelect fuel (choose number):\n", PAD);
            for (int i = 0; i < fuelCount; ++i) printf("%s%d. %s - %.2f per liter\n", PAD, i + 1, fuels[i].name, fuels[i].price);
            printf("%s%d. Finish and Checkout\n", PAD, fuelCount + 1);
            printf("%s0. Undo (Back to Mode)\n", PAD);
            printf("%s-1. Reset Transaction\n", PAD);

            int choice;
            printf("%sChoice: ", PAD);

            choice = getStrictInt(-1, fuelCount + 1);

            if (choice == -1) goto transaction_reset;
            if (choice == 0) goto step_purchase_mode;

            if (choice < 0) {
                printf("%sInvalid input.\n", PAD); pauseMessage(); continue;
            }

            if (choice == fuelCount + 1) {
                if (itemCount == 0) {
                    printf("%sCart is empty. Please add fuel.\n", PAD);
                    pauseMessage();
                    continue;
                }
                break;
            }

            if (itemCount >= MAX_ITEMS) break;

            double liters;
            printf("%sEnter liters for %s (0=Undo, -1=Reset): ", PAD, fuels[choice - 1].name);

            liters = getStrictDouble(-1.0);

            if (liters > 0.0) {
                if (!checkStockForPurchase(fuels[choice - 1].name, liters)) {
                    pauseMessage();
                    continue; /* Stock check failed (either low stock + delay or just pending delivery) */
                }
            }

            if (liters == -1.0) goto transaction_reset;
            if (liters == 0.0) continue;

            if (liters < 0.0) {
                printf("%sInvalid liters. Numeric value > 0 only.\n", PAD);
                pauseMessage();
                continue;
            }

            Item it;
            it.fuelIndex = choice - 1;
            it.liters = liters;
            it.pricePerLiter = fuels[it.fuelIndex].price;
            it.subtotal = it.liters * it.pricePerLiter;
            items[itemCount++] = it;

            char more = 'n';
            while(1) {
                printf("%sAdded. Add more? (y/n): ", PAD);
                char line[256];
                if (!fgets(line, sizeof(line), stdin)) { more = 'n'; break; }
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) == 1) {
                    if (line[0] == 'y' || line[0] == 'Y') { more = 'y'; break; }
                    if (line[0] == 'n' || line[0] == 'N') { more = 'n'; break; }
                }
                printf("%sInvalid. Enter 'y' or 'n'.\n", PAD);
            }
            if (more == 'n') break;
        }
    }

    if (itemCount == 0) {
        printf("%sTransaction cancelled.\n", PAD);
        pauseMessage();
        return;
    }

    double subtotal = 0.0;
    for (int i = 0; i < itemCount; ++i) subtotal += items[i].subtotal;

    char haveCard = 'n';
    int loyaltyValid = 0;
    double discount = 0.0;

    attempts = 0;
    while (1) {
        if (attempts >= MAX_ATTEMPTS) { haveCard = 'n'; break; }
        printf("\n%sDo you have a Loyalty Card? (y/n, 0=Undo to Cart): ", PAD);
        char line[256];
        if (!fgets(line, sizeof(line), stdin)) { haveCard = 'n'; break; }
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "0") == 0) {
            if (purchaseMode == 1) goto step_cash_entry;
            else goto step_fuel_loop;
        }

        if (strlen(line) == 1) {
            char c = line[0];
            if (c == 'y' || c == 'Y') { haveCard = 'y'; break; }
            if (c == 'n' || c == 'N') { haveCard = 'n'; break; }
        }
        attempts++;
        printf("%sInvalid input. Enter 'y' or 'n'.\n", PAD);
    }

    if (haveCard == 'y') {
        attempts = 0;
        while(1) {
            if (attempts >= MAX_ATTEMPTS) { printf("%sProceeding without discount.\n", PAD); pauseMessage(); break; }
            printf("%sEnter Loyalty Card code (0=Skip): ", PAD);
            unsigned long long code = getStrictULL();

            if (code == 0) break;

            int loyaltyStatus = isLoyaltyCodeValid(customerID, code);
            if (loyaltyStatus == 1) {
                loyaltyValid = 1;
                printf("%sLoyalty recognized. 10%% Discount.\n", PAD);
                pauseMessage();
                break;
            } else if (loyaltyStatus == -1) {
                printf("%sCard Expired.\n", PAD);
                pauseMessage();
                break;
            } else {
                attempts++;
                printf("%sInvalid Code. (%d left)\n", PAD, MAX_ATTEMPTS - attempts);
                pauseMessage();
            }
        }
    }

    /* --- NEW LOGIC: SENIOR CITIZEN / PWD --- */
    /*  */
    int isSenior = 0;
    attempts = 0;
    while (1) {
        if (attempts >= MAX_ATTEMPTS) { break; }
        printf("\n%s%sIs the customer a Senior Citizen or PWD? (y/n):%s ", PAD, CYAN, RESET);
        char line[256];
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 1) {
            if (tolower(line[0]) == 'y') {
                isSenior = 1;
                /* Senior Discount overrides Loyalty because it is usually higher/mandatory */
                if(loyaltyValid) {
                    printf("%s%sNote: Senior Discount overrides Loyalty Discount.%s\n", PAD, YELLOW, RESET);
                    loyaltyValid = 0;
                }
                break;
            }
            if (tolower(line[0]) == 'n') { isSenior = 0; break; }
        }
        printf("%sInvalid input. Enter 'y' or 'n'.\n", PAD);
        attempts++;
    }

    double tax = 0.0;
    if(isSenior) {
        /* Senior Logic: Tax is 0 (VAT Exempt), Discount is 20% of Subtotal */
        tax = 0.0;
        discount = subtotal * SC_PWD_DISCOUNT;
    } else {
        /* Regular Logic: Tax is 12% of Subtotal, Discount is 10% of (Subtotal+Tax) */
        tax = subtotal * TAX_RATE;
        if (loyaltyValid) discount = (subtotal + tax) * 0.10;
    }

    double grandTotal = subtotal + tax - discount;

    /* --- SHIFT SESSION UPDATE --- */
    if (session) {
        for(int i=0; i<itemCount; i++) {
            session->sessionLiters[items[i].fuelIndex] += items[i].liters;
        }
        session->sessionCashExpected += grandTotal;
        session->customersServed++; // New Counter
    }

    recordCustomer(customerID, customerName, grandTotal);
    logCashierActivity(session->cashierName, "Processed Sale");

    unsigned long long existingCode = getLoyaltyCode(customerID);
    /* Only ask to assign new card if they don't have one and didn't just renew it */
    if (existingCode == 0) {
        Customer updated = getCustomerDataByID(customerID);
        if (checkLoyaltyEligibility(updated.purchaseCount, updated.totalSpent)) {
            printf("\n%sCONGRATS: Eligible for Loyalty Card!\n", PAD);

            char wantPhysical = 'n';
            while (1) {
                printf("%sDo you want a physical card? (y/n): ", PAD);
                char line[256];
                if (!fgets(line, sizeof(line), stdin)) break;
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) == 1) {
                    if (tolower(line[0]) == 'y') { wantPhysical = 'y'; break; }
                    if (tolower(line[0]) == 'n') { wantPhysical = 'n'; break; }
                }
                printf("%sInvalid input. Enter 'y' or 'n'.\n", PAD);
            }

            unsigned long long newCode = assignLoyaltyCode(customerID, customerName);
            printf("%sNew Code Assigned: %llu\n", PAD, newCode);

            if (wantPhysical == 'y') {
                #ifdef _WIN32
                system(BARCODES_DIR_CMD_WIN);
                #else
                system(BARCODES_DIR_CMD_UNIX);
                #endif
                char codeString[32];
                snprintf(codeString, sizeof(codeString), "%llu", newCode);
                char barcodeFilename[256];
                snprintf(barcodeFilename, sizeof(barcodeFilename), BASE_BARCODE_PATH "loyalty_barcode_%04d.png", customerID);

                generateBarcodeImage(codeString, barcodeFilename);

                printf("%sOpening visual barcode in Photo Viewer...\n", PAD);
                openImageInViewer(barcodeFilename);
            }
            pauseMessage();
        }
    }

    clearScreen();
    printReceiptFormatted(stdout, session->cashierName, customerID, customerName, items, itemCount, subtotal, tax, discount, grandTotal, 0.0, 0.0, isSenior);

    double cash;
    if (purchaseMode == 1) {
        cash = cashAvailable;
        printf("\n%sCash Provided: PHP %.2f\n", PAD, cash);
    } else {
        attempts = 0;
        while(1) {
            if (attempts >= MAX_ATTEMPTS) goto transaction_reset;
            printf("\n%sEnter cash amount (0=Undo to Cart, -1=Reset): ", PAD);

            cash = getStrictDouble(-1.0);

            if (cash == -1.0) goto transaction_reset;
            if (cash == 0.0) goto step_fuel_loop;

            if (cash < grandTotal) {
                attempts++;
                printf("%sInsufficient cash. Need PHP %.2f.\n", PAD, grandTotal);
                pauseMessage();
                continue;
            }
            break;
        }
    }

    double change = cash - grandTotal;
    char filename[256];
    generateReceiptFilename(filename, sizeof(filename), customerID, customerName);
    FILE* rf = safeOpen(filename, "w");
    if (rf) {
        printReceiptFormatted(rf, session->cashierName, customerID, customerName, items, itemCount, subtotal, tax, discount, grandTotal, cash, change, isSenior);
        fclose(rf);
        strcpy(lastReceiptFilename, filename);
        printf("\n%sReceipt saved as: %s\n", PAD, filename);
        printReceiptToPrinter(filename);

        for (int i = 0; i < itemCount; i++) {
            deductStock(
                fuels[items[i].fuelIndex].name,
                items[i].liters
            );
        }
    }
    else {
        printf("\n%s%s[ERROR] Could not save receipt file!%s\n", PAD, RED, RESET);
        printf("%sCheck if the 'receipts' folder exists and has write permissions.\n", PAD);
    }
    pauseMessage();
}

void performZRead(ShiftSession* session) {
    system(CLEAR_SCREEN);
    printf("\n%s===== Z-READ (END OF SHIFT) =====\n", PAD);
    printf("%sCashier: %s\n", PAD, session->cashierName);
    char timeStr[64];
    struct tm* t = localtime(&session->loginTime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);
    printf("%sLogin Time: %s\n", PAD, timeStr);
    printf("%s---------------------------------\n", PAD);

    printf("%sLiters Sold:\n", PAD);
    int soldAny = 0;
    for(int i=0; i<fuelCount; i++) {
        if(session->sessionLiters[i] > 0) {
            printf("%s- %s: %.2f L\n", PAD, fuels[i].name, session->sessionLiters[i]);
            soldAny = 1;
        }
    }
    if(!soldAny) printf("%s(None)\n", PAD);

    printf("%s---------------------------------\n", PAD);
    printf("%sCustomers Served: %d\n", PAD, session->customersServed); // New Output
    printf("%sEXPECTED CASH: PHP %.2f\n", PAD, session->sessionCashExpected);

    printf("\n%sEnter ACTUAL CASH in drawer: PHP ", PAD);
    double actual = getStrictDouble(0.0);

    double diff = actual - session->sessionCashExpected;

    printf("%s---------------------------------\n", PAD);
    if(diff == 0) printf("%sStatus: %sBALANCED%s\n", PAD, GREEN, RESET);
    else if(diff < 0) printf("%sStatus: %sSHORTAGE (%.2f)%s\n", PAD, RED, diff, RESET);
    else printf("%sStatus: %sOVERAGE (+%.2f)%s\n", PAD, BLUE, diff, RESET);

    // Log Report
    FILE* fp = fopen(SHIFT_REPORT_FILE, "a");
    if(fp) {
        time_t now = time(NULL);
        char endStr[64];
        struct tm* te = localtime(&now);
        strftime(endStr, sizeof(endStr), "%Y-%m-%d %H:%M:%S", te);

        fprintf(fp, "[Z-READ] End: %s | Cashier: %s | Customers: %d | Expected: %.2f | Actual: %.2f | Diff: %.2f\n",
            endStr, session->cashierName, session->customersServed, session->sessionCashExpected, actual, diff);
        fclose(fp);
        printf("\n%sReport saved to %s\n", PAD, SHIFT_REPORT_FILE);
    }
    pauseMessage();
}

/* =========================================================================
 * 10. ADMIN SYSTEM
 * ========================================================================= */

void ensureAdminFileExists(void) {
    FILE* fp = fopen(ADMIN_FILE, "r");
    if(!fp) {
        fp = fopen(ADMIN_FILE, "w");
        if(fp) {
            /* Create Master Admin */
            fprintf(fp, "Zack zack123\n");
            fclose(fp);
        }
    } else {
        fclose(fp);
    }
}

int verifyAdmin(const char* user, const char* pass) {
    ensureAdminFileExists();
    FILE* fp = fopen(ADMIN_FILE, "r");
    if(!fp) return 0;

    char u[32], p[32];
    while(fscanf(fp, "%s %s", u, p) == 2) {
        if(strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void editGasPrices(void) {
    while(1) {
        system(CLEAR_SCREEN);
        printf("\n%s%s===== EDIT GAS PRICES =====%s\n", PAD, MAGENTA, RESET);
        printf("%s%s%-5s | %-20s | %-10s%s\n", PAD, CYAN, "No.", "Fuel Type", "Price", RESET);
        printf("%s---------------------------------------------\n", PAD);

        // Display current prices
        for(int i=0; i<fuelCount; i++) {
            printf("%s%-5d | %-20s | PHP %.2f\n", PAD, i+1, fuels[i].name, fuels[i].price);
        }
        printf("%s---------------------------------------------\n", PAD);
        printf("%s0. Finish and Save\n", PAD);
        printf("%sSelect fuel to edit: ", PAD);

        int choice = getStrictInt(0, fuelCount);

        if (choice == -1) {
            printf("%sInvalid input.\n", PAD);
            SLEEP(500);
            continue;
        }

        if (choice == 0) {
            // Save and Exit
            savePricesToFile();
            printf("\n%sPrices saved successfully!\n", PAD);
            pauseMessage();
            return;
        }

        // Edit specific fuel
        int idx = choice - 1; // Array is 0-indexed, display is 1-indexed
        printf("\n%sEnter new price for %s (Current: %.2f): ", PAD, fuels[idx].name, fuels[idx].price);
        double newPrice = getStrictDouble(0.01); // Minimum price 0.01

        if (newPrice > 0) {
            fuels[idx].price = newPrice;
            printf("%sPrice updated.\n", PAD);
            SLEEP(500); // Brief pause to see confirmation
        } else {
            printf("%sInvalid price. No change made.\n", PAD);
            SLEEP(1000);
        }
    }
}

/* Helper to check if name is in admins.txt */
int isNameInAdminsFile(const char* name) {
    FILE* fp = fopen(ADMIN_FILE, "r");
    if(!fp) return 0;
    char u[32], p[32];
    while(fscanf(fp, "%s %s", u, p) == 2) {
        if(strcmp(u, name) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

/* Helper to check if name is in cashiers array */
int isNameInCashiersArray(const char* name) {
    for(int i=0; i<cashierCount; i++) {
        #ifdef _WIN32
        if(_stricmp(cashiers[i].name, name) == 0) return 1;
        #else
        if(strcasecmp(cashiers[i].name, name) == 0) return 1;
        #endif
    }
    return 0;
}

/* 1 = Cashier, 2 = Admin */
void viewLogsFiltered(int mode) {
    system(CLEAR_SCREEN);
    /* Set correct filename based on mode */
    const char* targetFile = (mode == 1) ? CASHIER_LOG_FILE : ADMIN_LOG_FILE;

    printf("\n%s===== %s LOGS =====\n", PAD, (mode == 1 ? "CASHIER" : "ADMIN"));

    FILE* fp = fopen(targetFile, "r");
    if(!fp) {
        printf("%sNo logs found.\n", PAD);
    } else {
        char buffer[256];
        int foundAny = 0;
        while(fgets(buffer, sizeof(buffer), fp)) {
            // Parse Name: "[Date] User: NAME | Action..." or "[Date] Admin: NAME | Action..."
            // We'll use a slightly looser check: if it's the right file, we assume lines are valid.
            // But to be extra safe with the filter request:

            char* userPtr = NULL;
            if (mode == 1) userPtr = strstr(buffer, "User: ");
            else userPtr = strstr(buffer, "Admin: ");

            if(userPtr) {
                userPtr += (mode == 1 ? 6 : 7); // Skip "User: " or "Admin: "
                char tempName[32] = {0};
                int i = 0;
                while(*userPtr != ' ' && *userPtr != '|' && *userPtr != '\0' && i < 31) {
                    tempName[i++] = *userPtr++;
                }
                tempName[i] = '\0';

                // Basic validation: Just show all lines from the correct file
                printf("%s%s", TABLE_PAD, buffer);
                foundAny = 1;
            }
        }
        if(!foundAny) printf("%sNo logs found in file.\n", TABLE_PAD);
        fclose(fp);
    }
    pauseAndReturnToMenu();
}

void cashierQuarry(void) {
    system(CLEAR_SCREEN);
    printf("\n%s===== CASHIER QUARRY =====\n", PAD);
    // List Cashiers
    printf("%s%sRegistered Cashiers:%s\n", PAD, BOLD, RESET);
    for(int i=0; i<cashierCount; i++) {
        printf("%s- %s (ID: %d)\n", PAD, cashiers[i].name, cashiers[i].id);
    }
    printf("%s----------------------------\n", PAD);

    printf("%sEnter Cashier ID to analyze (0 to back): ", PAD);
    int id = getStrictInt(0, 9999);
    if(id == 0) return;

    // Find Name from ID
    const char* targetName = NULL;
    for(int i=0; i<cashierCount; i++) {
        if(cashiers[i].id == id) { targetName = cashiers[i].name; break; }
    }

    if(!targetName) { printf("%sCashier ID not found.\n", PAD); pauseMessage(); return; }

    printf("\n%sAnalyzing shift reports for %s...\n", PAD, targetName);

    FILE* fp = fopen(SHIFT_REPORT_FILE, "r");
    if(!fp) { printf("%sNo shift reports found.\n", PAD); pauseMessage(); return; }

    double totalExpected = 0.0;
    int totalCustomers = 0;
    int shiftCount = 0;
    char buffer[512];

    // Parse File
    // Format: [Z-READ] End: ... | Cashier: Name | Customers: X | Expected: Y ...
    while(fgets(buffer, sizeof(buffer), fp)) {
        if(strstr(buffer, targetName)) { // Simple check if name exists in line
            // Check if it's actually the "Cashier: Name" part to avoid partial matches
            char checkStr[64];
            snprintf(checkStr, 64, "Cashier: %s", targetName);
            if(strstr(buffer, checkStr)) {
                shiftCount++;

                // Parse Expected Sales
                char* expPtr = strstr(buffer, "Expected: ");
                if(expPtr) {
                    totalExpected += atof(expPtr + 10);
                }

                // Parse Customers (New Format)
                char* custPtr = strstr(buffer, "Customers: ");
                if(custPtr) {
                    totalCustomers += atoi(custPtr + 11);
                }
            }
        }
    }
    fclose(fp);

    printf("%s----------------------------\n", PAD);
    printf("%sTotal Shifts:       %d\n", PAD, shiftCount);
    printf("%sTotal Customers:    %d\n", PAD, totalCustomers);
    printf("%sTotal Sales:        PHP %.2f\n", PAD, totalExpected);
    printf("%s----------------------------\n", PAD);
    pauseMessage();
}

void manageAdmins(void) {
    while(1) {
        system(CLEAR_SCREEN);
        printf("\n%s%s===== MANAGE ADMINS =====%s\n", PAD, MAGENTA, RESET);
        printf("%s%s1.%s List Admins\n", PAD, YELLOW, RESET);
        printf("%s%s2.%s Add Admin\n", PAD, YELLOW, RESET);
        printf("%s%s3.%s Delete Admin\n", PAD, YELLOW, RESET);
        printf("%s%s0.%s Back\n", PAD, RED, RESET);
        printf("%sChoice: ", PAD);
        int c = getStrictInt(0, 3);
        if(c == 0) break;

        if(c == 1) {
            FILE* fp = fopen(ADMIN_FILE, "r");
            char u[32], p[32];
            printf("\n%sList of Admins:\n", PAD);
            if(fp) {
                while(fscanf(fp, "%s %s", u, p) == 2) printf("%s- %s\n", PAD, u);
                fclose(fp);
            }
            pauseMessage();
        }
        else if(c == 2) {
            char nu[32], np[32];
            /* --- FIXED: REPLACED scanf WITH FGETS LOGIC --- */
            printf("%sNew Username: ", PAD);
            if (fgets(nu, sizeof(nu), stdin)) { nu[strcspn(nu, "\n")] = 0; }

            printf("%sNew Password: ", PAD);
            if (fgets(np, sizeof(np), stdin)) { np[strcspn(np, "\n")] = 0; }

            /* Check duplicate */
            FILE* fp = fopen(ADMIN_FILE, "a+");
            int exists = 0;
            char u[32], p[32];
            rewind(fp);
            while(fscanf(fp, "%s %s", u, p) == 2) {
                if(strcmp(u, nu) == 0) exists = 1;
            }
            if(exists) printf("%sAdmin already exists.\n", PAD);
            else {
                fprintf(fp, "%s %s\n", nu, np);
                printf("%sAdmin Added.\n", PAD);
                /* CHANGED: Use Admin Logger */
                logAdminActivity("System", "New Admin Created");
            }
            fclose(fp);
            pauseMessage();
        }
        else if(c == 3) {
            char du[32];
            /* --- FIXED: REPLACED scanf WITH FGETS LOGIC --- */
            printf("%sUsername to delete: ", PAD);
            if (fgets(du, sizeof(du), stdin)) { du[strcspn(du, "\n")] = 0; }

            if(strcmp(du, "Zack") == 0) {
                printf("%sCannot delete Master Admin.\n", PAD);
            } else {
                FILE* fp = fopen(ADMIN_FILE, "r");
                FILE* tp = fopen(BASE_DATA_PATH "temp_admins.txt", "w");
                char u[32], p[32];
                int del = 0;
                if(fp && tp) {
                    while(fscanf(fp, "%s %s", u, p) == 2) {
                        if(strcmp(u, du) != 0) fprintf(tp, "%s %s\n", u, p);
                        else del = 1;
                    }
                    fclose(fp); fclose(tp);
                    remove(ADMIN_FILE);
                    rename(BASE_DATA_PATH "temp_admins.txt", ADMIN_FILE);
                    if(del) {
                        printf("%sDeleted.\n", PAD);
                        logAdminActivity("System", "Admin Deleted");
                    } else printf("%sUser not found.\n", PAD);
                }
            }
            pauseMessage();
        }
    }
}

/* --- NEW FEATURE: ASCII ANALYTICS DASHBOARD --- */
void viewSalesAnalytics(void) {
        system(CLEAR_SCREEN);
        printf("\n%s%s===== SALES ANALYTICS DASHBOARD =====%s\n", PAD, MAGENTA, RESET);

        FILE* fp = fopen(STOCK_LOG_FILE, "r");
        if (!fp) {
            printf("%sNo data available.\n", PAD);
            pauseMessage();
            return;
        }

        double totals[4] = {0.0}; // Stores total liters sold for each fuel type
        char line[256];

        /* 1. Parse the Log File */
        while (fgets(line, sizeof(line), fp)) {
            /* We only care about lines tagged with "SALE" or "VOID-RETURN" */
            /* SALE entries are negative numbers, so we flip them to positive for the chart */
            if (strstr(line, "SALE")) {
                for (int i = 0; i < fuelCount; i++) {
                    if (strstr(line, fuels[i].name)) {
                        /* Find the "Amount:" part */
                        char* amtPtr = strstr(line, "Amount: ");
                        if (amtPtr) {
                            double val = atof(amtPtr + 8); // Skip "Amount: "
                            totals[i] += (val * -1.0); // Convert -50.00 to 50.00
                        }
                        break;
                    }
                }
            }
        }
        fclose(fp);

        /* 2. Find Max Value for Scaling */
        double maxVal = 1.0; // Avoid divide by zero
        double grandTotal = 0.0;
        int bestSellerIdx = -1;
        double bestSellerVol = -1.0;

        for (int i = 0; i < fuelCount; i++) {
            if (totals[i] > maxVal) maxVal = totals[i];
            grandTotal += totals[i];
            if (totals[i] > bestSellerVol) {
                bestSellerVol = totals[i];
                bestSellerIdx = i;
            }
        }

        /* 3. Render the Chart */
        printf("\n");
        int maxBarWidth = 30; // Maximum characters for the bar

        for (int i = 0; i < fuelCount; i++) {
            int barLen = (int)((totals[i] / maxVal) * maxBarWidth);

            printf("%s%-20s : [%s", PAD, fuels[i].name, CYAN);
            for (int b = 0; b < barLen; b++) printf("#");
            for (int b = barLen; b < maxBarWidth; b++) printf(" ");
            printf("%s] %s%.2f L%s\n", RESET, YELLOW, totals[i], RESET);
        }

        /* 4. Summary Stats */
        printf("\n%s----------------------------------------\n", PAD);
        printf("%sTotal Volume Sold : %s%.2f Liters%s\n", PAD, WHITE, grandTotal, RESET);
        if (bestSellerIdx != -1) {
            printf("%sBest Performing   : %s%s%s\n", PAD, GREEN, fuels[bestSellerIdx].name, RESET);
        }
        printf("%s----------------------------------------\n", PAD);

        pauseMessage();
}

void adminMenu(const char* adminName) {
    /* Check prices on first run */
    if(!arePricesSet()) {
        printf("\n%s%sATTENTION: Gas prices are not set (0.00).%s\n", PAD, RED, RESET);
        printf("%sYou must set prices before using the system.\n", PAD);
        editGasPrices();
    }

    while(1) {
        system(CLEAR_SCREEN);
        printf("\n%s%s===== ADMIN PANEL (User: %s) =====%s\n", PAD, MAGENTA, adminName, RESET);
        printf("\n%s%s1.%s View Stock History%s\n", PAD, GREEN, GREEN, RESET);
        printf("\n%s%s2.%s View Stock Levels%s\n", PAD, GREEN, GREEN, RESET);
        printf("\n%s%s3.%s Sales Analytics%s\n", PAD, CYAN, CYAN, RESET);
        printf("\n%s%s4.%s Export Data to Excel%s\n", PAD, CYAN, CYAN, RESET);
        printf("\n%s%s5.%s View Cashier Logs%s\n", PAD, YELLOW, YELLOW, RESET);
        printf("\n%s%s6.%s View Admin Logs%s\n", PAD, YELLOW, YELLOW, RESET);
        printf("\n%s%s7.%s Edit Gas Prices%s\n", PAD, GREEN, GREEN, RESET);
        printf("\n%s%s8.%s Customer Quarry%s\n", PAD, YELLOW, YELLOW, RESET);
        printf("\n%s%s9.%s Cashier Quarry%s\n", PAD, YELLOW, YELLOW, RESET);
        printf("\n%s%s10.%s Manual Restock%s\n", PAD, GREEN, GREEN, RESET);
        printf("\n%s%s11.%s Manage Admins%s\n", PAD, CYAN, CYAN, RESET);
        printf("\n%s%s12.%s Logout%s\n", PAD, YELLOW, YELLOW, RESET);
        printf("\n%s%s13.%s Exit System%s\n", PAD, RED, RED, RESET);
        printf("\n%sChoice: ", PAD);

        int c = getStrictInt(1, 13);
        if(c == -1) { printf("%sInvalid Input.\n", PAD); pauseMessage(); continue; }

        switch(c) {
            case 1:
            {
                system(CLEAR_SCREEN);
                FILE* logF = fopen(STOCK_LOG_FILE, "r");
                printf("\n%s--- Stock Transaction History ---\n", TABLE_PAD);
                if (logF) {
                    char buffer[256];
                    while(fgets(buffer, sizeof(buffer), logF)) printf("%s%s", TABLE_PAD, buffer);
                    fclose(logF);
                } else {
                    printf("%sNo transaction history available.\n", TABLE_PAD);
                }
                pauseAndReturnToMenu();
            }
            break;
            case 2: viewGasolineStock(); break;
            case 3: viewSalesAnalytics(); break;
            case 4: exportToCSV(); break;
            case 5: viewLogsFiltered(1); break;
            case 6: viewLogsFiltered(2); break;
            case 7: editGasPrices(); break;
            case 8: customerQuarry(1); break;
            case 9: cashierQuarry(); break;
            case 10: manualRestockMenu(adminName); break;
            case 11: manageAdmins(); break;
            case 12: return;
            case 13: exit(0);
        }
    }
}

Cashier* cashierLogin(void) {
    int attempts = 0;
    while (1) {
        if (attempts >= MAX_ATTEMPTS) {
            printf("\n%s%sTOO MANY FAILED ATTEMPTS.%s\n", PAD, RED, RESET);
            printf("%sReturning to Login Selection for security...\n", PAD);
            attempts = 0;
            pauseMessage();
            return NULL;
        }

        clearScreen();
        printf("\n");
        printf("%s%s _______ _____ _____ _____ _____ _____ ___ __________ %s\n", PAD, MAGENTA, RESET);
        printf("%s%s|       _____ _____ _____ _____ _____ _____          | %s\n", PAD, YELLOW, RESET);
        printf("%s%s|______/-|   __|  _  |__   |  _  |  |  |   __|---/___| %s\n", PAD, RED, RESET);
        printf("%s%s|_____/--|__   |     |   __|     |   <_    __|--/____| %s\n", PAD, BLUE, RESET);
        printf("%s%s|____/---|_____|__|__|_____|__|__|__|__|_____|-/_____| %s\n", PAD, RED, RESET);
        printf("%s%s|             === SAZAKE GAS STATION ===             | %s\n", PAD, CYAN, RESET);
        printf("%s%s|_____ _____ _____ _____ _____ _____ _____ __________| %s\n", PAD, YELLOW, RESET);
        printf("%s%s******************************************************%s\n", PAD, RED, RESET);
        printf("%s%s**************** Power Your Journey ******************%s\n", PAD, BLUE, RESET);
        printf("%s%s******************************************************%s\n\n", PAD, RED, RESET);

        printf("%s%s=== LOGIN SELECTION ===%s\n", PAD, YELLOW, RESET);
        printf("%s%s1.%s Cashier Login\n", PAD, GREEN, GREEN, RESET);
        printf("%s%s2.%s Admin Login\n", PAD, MAGENTA, MAGENTA, RESET);
        printf("%s%s3.%s Exit System\n", PAD, RED, RED, RESET);
        printf("%s%sChoice: ", PAD, CYAN, RESET);

        int opt = getStrictInt(1, 3);

        if (opt == -1) {
            attempts++;
            printf("%sInvalid input. Please enter 1, 2, or 3. (%d attempts left)\n", PAD, MAX_ATTEMPTS - attempts);
            pauseMessage();
            continue;
        }

        if (opt == 3) exit(0);

        if (opt == 2) {
            char u[32], p[32];
            printf("\n%sAdmin Username: ", PAD);
            if (fgets(u, sizeof(u), stdin)) {
                u[strcspn(u, "\n")] = 0;
            }

            printf("%sAdmin Password: ", PAD);
            getPasswordInput(p, 32);
            if(verifyAdmin(u, p)) {
                logAdminActivity(u, "Admin Login");
                adminMenu(u);
            } else {
                printf("%sInvalid Admin Credentials.\n", PAD);
                logAdminActivity(u, "Failed Admin Login Attempt");
                pauseMessage();
            }
            continue;
        }

        if (opt == 1) {
            printf("\n%s%s1. Manual Login\n", PAD, CYAN, RESET);
            printf("%s%s2. Scan ID\n", PAD, GREEN, RESET);
            printf("%s%s0. Back\n", PAD, YELLOW, RESET);
            printf("%sMethod: ", PAD);
            int m = getStrictInt(0, 2);
            if(m == 0) continue;

            if (m == 1) {
                char nameIn[64], passIn[64];

                printf("%sEnter name: ", PAD);
                if (!fgets(nameIn, sizeof(nameIn), stdin)) continue;
                nameIn[strcspn(nameIn, "\n")] = '\0';
                printf("%sEnter password: ", PAD);
                getPasswordInput(passIn, sizeof(passIn));

                for (int i = 0; i < cashierCount; ++i) {
                    #ifdef _WIN32
                    if (_stricmp(nameIn, cashiers[i].name) == 0 && strcmp(passIn, cashiers[i].password) == 0)
                    #else
                    if (strcasecmp(nameIn, cashiers[i].name) == 0 && strcmp(passIn, cashiers[i].password) == 0)
                    #endif
                    {
                        logCashierActivity(cashiers[i].name, "Cashier Login");
                        return &cashiers[i];
                    }
                }
                printf("%sInvalid credentials.\n", PAD);
                pauseMessage();
            } else if (m == 2) {
                printf("%sEnter ID: ", PAD);
                int scanID = getStrictInt(0, 9999);
                for (int i = 0; i < cashierCount; ++i) {
                    if (scanID == cashiers[i].id) {
                        logCashierActivity(cashiers[i].name, "Cashier Login via ID");
                        return &cashiers[i];
                    }
                }
                printf("%sUnknown ID.\n", PAD);
                pauseMessage();
            }
        }
    }
}

void reprintLastReceipt(void) {
        if (strlen(lastReceiptFilename) == 0) {
            printf("\n%s%s[ERROR] No receipt generated in this session yet.%s\n", PAD, RED, RESET);
        } else {
            printf("\n%s%sReprinting last receipt...%s\n", PAD, GREEN, RESET);
            printf("%sFile: %s\n", PAD, lastReceiptFilename);
            printReceiptToPrinter(lastReceiptFilename);
        }
        pauseMessage();
}

void returnStock(const char* fuelName, double litersReturned) {
    int targetID = -1;
    for(int i=0; i<fuelCount; i++) {
        #ifdef _WIN32
        if (_stricmp(fuels[i].name, fuelName) == 0)
        #else
        if (strcasecmp(fuels[i].name, fuelName) == 0)
        #endif
        {
            targetID = 101 + i;
            break;
        }
    }
    if (targetID == -1) return;

    GasolineStock* stocks = (GasolineStock*)malloc(MAX_TYPES * sizeof(GasolineStock));
    if(!stocks) return;

    int count = loadStocksFromFile(stocks);
    processPendingDeliveries(stocks, count);

    int idx = findStockIndexByID(stocks, count, targetID);

    if (idx != -1) {
        stocks[idx].currentStock += litersReturned;
        saveStocksToFile(stocks, count);
        logStockTransaction(targetID, fuelName, litersReturned, "VOID-RETURN");
    }
    free(stocks);
}

int voidCustomerSale(int customerID, double amountToDeduct) {
    Customer* customers = (Customer*)malloc(MAX_CUSTOMERS * sizeof(Customer));
    if (!customers) return 0;

    int customerCount = 0;
    int found = 0;

    FILE* fp = fopen(CUSTOMER_FILE, "r");
    if (fp) {
        while (customerCount < MAX_CUSTOMERS &&
            fscanf(fp, "%d \"%255[^\"]\" %d %lf",
                &customers[customerCount].id,
                customers[customerCount].name,
                &customers[customerCount].purchaseCount,
                &customers[customerCount].totalSpent) == 4) {
            customerCount++;
        }
        fclose(fp);
    }

    for (int i = 0; i < customerCount; i++) {
        if (customers[i].id == customerID) {
            customers[i].purchaseCount--;
            customers[i].totalSpent -= amountToDeduct;

            if(customers[i].purchaseCount < 0) customers[i].purchaseCount = 0;
            if(customers[i].totalSpent < 0.0) customers[i].totalSpent = 0.0;

            found = 1;
            break;
        }
    }

    if (found) {
        fp = fopen(CUSTOMER_FILE, "w");
        if (fp) {
            for (int i = 0; i < customerCount; i++) {
                fprintf(fp, "%d \"%s\" %d %.2f\n",
                    customers[i].id,
                    customers[i].name,
                    customers[i].purchaseCount,
                    customers[i].totalSpent);
            }
            fclose(fp);
        }
    }
    free(customers);
    return found;
}

void voidTransaction(ShiftSession* session) {
    system(CLEAR_SCREEN);
    printf("\n%s%s===== VOID TRANSACTION (MANAGER OVERRIDE) =====%s\n", PAD, RED, RESET);
    printf("%sThis action will reverse sales, stock, and customer records.\n", PAD);

    char u[32], p[32];
    printf("\n%s%sAdmin Authorization Required.%s\n", PAD, YELLOW, RESET);
    printf("%sAdmin Username: ", PAD);
    if (!fgets(u, sizeof(u), stdin)) return;
    u[strcspn(u, "\n")] = 0;

    printf("%sAdmin Password: ", PAD);
    getPasswordInput(p, 32);

    if(!verifyAdmin(u, p)) {
        printf("%s%sAuthorization Failed.%s\n", PAD, RED, RESET);
        logAdminActivity(u, "Failed Void Auth");
        pauseMessage();
        return;
    }

    logAdminActivity(u, "Authorized Void Transaction");

    printf("\n%s%s--- Enter Details from Receipt ---%s\n", PAD, CYAN, RESET);

    printf("%sCustomer ID to void: ", PAD);
    int custID = getStrictInt(0, 99999);
    if(custID <= 0) return;

    Customer c = getCustomerDataByID(custID);
    if(c.id == 0) {
        printf("%sCustomer ID not found.\n", PAD);
        pauseMessage();
        return;
    }
    printf("%sCustomer Found: %s\n", PAD, c.name);

    printf("\n%sSelect Fuel Type to return:\n", PAD);
    for(int i=0; i<fuelCount; i++) printf("%s%d. %s\n", PAD, i+1, fuels[i].name);
    printf("%sChoice: ", PAD);
    int fChoice = getStrictInt(1, fuelCount);
    if(fChoice == -1) return;
    int fIndex = fChoice - 1;

    printf("%sLiters to return to tank: ", PAD);
    double liters = getStrictDouble(0.0);

    printf("%sTotal Cash Amount to refund/void (PHP): ", PAD);
    double amount = getStrictDouble(0.0);

    printf("\n%s%sCONFIRM VOID? (y/n):%s ", PAD, RED, RESET);
    char confirm[256];
    fgets(confirm, sizeof(confirm), stdin);
    if(tolower(confirm[0]) != 'y') {
        printf("%sCancelled.\n", PAD);
        pauseMessage();
        return;
    }

    printf("\n%sProcessing...\n", PAD);

    returnStock(fuels[fIndex].name, liters);
    printf("%s- Stock Returned: %.2f L\n", PAD, liters);

    if(voidCustomerSale(custID, amount)) {
        printf("%s- Customer Record Updated ( -PHP %.2f )\n", PAD, amount);
    } else {
        printf("%s- Warning: Could not update customer file.\n", PAD);
    }

    printf("%sIs this from the CURRENT shift? (y/n): ", PAD);
    fgets(confirm, sizeof(confirm), stdin);
    if(tolower(confirm[0]) == 'y') {
        session->sessionLiters[fIndex] -= liters;
        if(session->sessionLiters[fIndex] < 0) session->sessionLiters[fIndex] = 0;

        session->sessionCashExpected -= amount;
        if(session->sessionCashExpected < 0) session->sessionCashExpected = 0;

        session->customersServed--;
        if(session->customersServed < 0) session->customersServed = 0;

        printf("%s- Current Session Adjusted.\n", PAD);
    }

    printf("\n%s%sTransaction Voided Successfully.%s\n", PAD, GREEN, RESET);

    char voidLog[128];
    snprintf(voidLog, sizeof(voidLog), "VOIDED: ID %d, %.2fL %s", custID, liters, fuels[fIndex].name);
    logCashierActivity(session->cashierName, voidLog);

    pauseMessage();
}

void exportToCSV(void) {
    system(CLEAR_SCREEN);
    printf("\n%s%s===== EXPORTING DATA... =====%s\n", PAD, CYAN, RESET);

    FILE* src = fopen(STOCK_LOG_FILE, "r");
    char stockCsvPath[256];
    sprintf(stockCsvPath, "%sstock_export.csv", BASE_DATA_PATH);

    FILE* dest = fopen(stockCsvPath, "w");

    if (src && dest) {
        fprintf(dest, "Timestamp,Transaction Type,Details,Amount Change\n");

        char line[512];
        while(fgets(line, sizeof(line), src)) {

            for(int i=0; line[i]; i++) {
                if(line[i] == '|') line[i] = ',';
                if(line[i] == '[' || line[i] == ']') line[i] = ' ';
            }
            fprintf(dest, "%s", line);
        }
        printf("%s%s[SUCCESS] Stock logs exported to: %s%s\n", PAD, GREEN, stockCsvPath, RESET);
        fclose(src);
        fclose(dest);
    } else {
        printf("%s%s[ERROR] Could not read Stock Logs.%s\n", PAD, RED, RESET);
    }

    src = fopen(SHIFT_REPORT_FILE, "r");
    char salesCsvPath[256];
    sprintf(salesCsvPath, "%ssales_export.csv", BASE_DATA_PATH);
    dest = fopen(salesCsvPath, "w");

    if (src && dest) {
        fprintf(dest, "Timestamp,Cashier,Customers Served,Expected Cash,Actual Cash,Difference\n");

        char line[512];
        while(fgets(line, sizeof(line), src)) {
            char cleanLine[512] = "";
            int cIdx = 0;

            char* startPtr = strstr(line, "End: ");
            if(!startPtr) startPtr = line;

            for(int i=0; startPtr[i]; i++) {
                if(startPtr[i] == '|') startPtr[i] = ',';
                if(startPtr[i] != '\n') cleanLine[cIdx++] = startPtr[i];
            }
            cleanLine[cIdx] = '\0';
            fprintf(dest, "%s\n", cleanLine);
        }
        printf("%s%s[SUCCESS] Sales logs exported to: %s%s\n", PAD, GREEN, salesCsvPath, RESET);
        fclose(src);
        fclose(dest);
    } else {
        printf("%s%s[ERROR] Could not read Shift Reports.%s\n", PAD, RED, RESET);
    }

    printf("\n%sYou can now open these files in Excel.\n", PAD);
    pauseMessage();
}

int main(void) {
    system(DATA_DIR_CMD_WIN);
    system(RECEIPTS_DIR_CMD_WIN);
    system(BARCODES_DIR_CMD_WIN);

    srand((unsigned int)(time(NULL) ^ (uintptr_t)&main));

    FILE* fp = fopen(CUSTOMER_FILE, "a"); if(fp) fclose(fp);
    FILE* idf = fopen(CUSTOMER_ID_FILE, "r");
    if(!idf) { idf = fopen(CUSTOMER_ID_FILE, "w"); if(idf){fprintf(idf, "1001\n"); fclose(idf);} }
    else fclose(idf);

    ensureAdminFileExists();
    loadPricesFromFile();

    while (1) {
        Cashier* logged = cashierLogin();
        if (!logged) continue;

        if(!arePricesSet()) {
            printf("\n%s%sSYSTEM LOCKED: Prices not set by Admin.%s\n", PAD, RED, RESET);
            printf("%sCashier cannot proceed.\n", PAD);
            pauseMessage();
            continue;
        }

        ShiftSession currentSession = {0};
        currentSession.cashierID = logged->id;
        strcpy(currentSession.cashierName, logged->name);
        currentSession.loginTime = time(NULL);

        while (1) {
            clearScreen();
            printf("\n%sLogged in as: %s%s%s (ID: %04d)\n", PAD, WHITE, logged->name, RESET, logged->id);
            printf("%s%s===== CASHIER MENU =====%s\n", PAD, MAGENTA, RESET);
            printf("%s%s1.%s Purchase Gas\n", PAD, YELLOW, RESET);
            printf("%s%s2.%s Customer Quarry\n", PAD, YELLOW, RESET);
            printf("%s%s3.%s Stock Management\n", PAD, YELLOW, RESET);
            printf("%s%s4.%s Reprint Last Receipt\n", PAD, YELLOW, RESET);
            printf("%s%s5.%s Void Transaction (Manager)\n", PAD, RED, RESET);
            printf("%s%s6.%s Logout (Z-Read)\n", PAD, YELLOW, RESET);
            printf("%s%s7.%s Exit\n", PAD, RED, RESET);
            printf("%sChoice: ", PAD);

            int opt = getStrictInt(1, 7);

            if (opt == -1) {
                 printf("%s%sInvalid choice. Please enter 1-7.\n", PAD, RED, RESET);
                 pauseMessage();
                 continue;
            }

            switch (opt) {
                case 1: purchaseGas(&currentSession); break;
                case 2: customerQuarry(0); break;
                case 3: stockMenu(); break;
                case 4: reprintLastReceipt(); break;
                case 5: voidTransaction(&currentSession); break;
                case 6:
                    logCashierActivity(logged->name, "Logout / Z-Read");
                    performZRead(&currentSession);
                    goto loop_end;
                case 7:
                    logCashierActivity(logged->name, "System Exit");
                    exit(0);
                default: printf("%sInvalid.\n", PAD);
            }
        }
        loop_end:;
    }
    return 0;
}
