// gen_data.cpp — Generate TPC-H subset: 20K customers, 30K orders, 50K lineitems
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static const char* SEGMENTS[] = {"BUILDING","AUTOMOBILE","MACHINERY","HOUSEHOLD","FURNITURE"};
static const char* NATIONS[]  = {"GERMANY","FRANCE","USA","BRAZIL","INDIA",
                                   "CHINA","RUSSIA","JAPAN","CANADA","EGYPT"};
static const char* STATUS[]   = {"O","F","P"};
static const char* PRIORITY[] = {"1-URGENT","2-HIGH","3-MEDIUM","4-NOT SPECIFIED","5-LOW"};
static const char* RETURNFLAG[]= {"N","R","A"};
static const char* LINESTATUS[]= {"O","F"};

int main() {
    srand((unsigned)time(nullptr));

    // ── customer.tbl ──────────────────────────────────────────────────────────
    FILE* fc = fopen("data/customer.tbl","w");
    if (!fc) { fprintf(stderr,"Cannot open data/customer.tbl\n"); return 1; }
    for (int i = 1; i <= 20000; i++) {
        int    custkey   = i;
        float  acctbal   = (rand()%999900) / 100.0f - 999.0f;
        int    nationkey = rand()%10;
        const char* seg  = SEGMENTS[rand()%5];
        fprintf(fc, "%d|Customer_%d|%.2f|%s|%d\n",
                custkey, custkey, acctbal, seg, nationkey);
    }
    fclose(fc);
    printf("Generated data/customer.tbl (20000 rows)\n");

    // ── orders.tbl ────────────────────────────────────────────────────────────
    FILE* fo = fopen("data/orders.tbl","w");
    if (!fo) { fprintf(stderr,"Cannot open data/orders.tbl\n"); return 1; }
    for (int i = 1; i <= 30000; i++) {
        int   orderkey  = i;
        int   custkey   = (rand()%20000)+1;
        const char* st  = STATUS[rand()%3];
        float total     = (rand()%50000000)/100.0f;
        const char* pri = PRIORITY[rand()%5];
        fprintf(fo, "%d|%d|%s|%.2f|%s\n",
                orderkey, custkey, st, total, pri);
    }
    fclose(fo);
    printf("Generated data/orders.tbl (30000 rows)\n");

    // ── lineitem.tbl ──────────────────────────────────────────────────────────
    FILE* fl = fopen("data/lineitem.tbl","w");
    if (!fl) { fprintf(stderr,"Cannot open data/lineitem.tbl\n"); return 1; }
    for (int i = 1; i <= 50000; i++) {
        int   orderkey  = (rand()%30000)+1;
        int   partkey   = (rand()%10000)+1;
        float price     = (rand()%10000000)/100.0f;
        float discount  = (rand()%11)/100.0f;
        const char* rf  = RETURNFLAG[rand()%3];
        const char* ls  = LINESTATUS[rand()%2];
        fprintf(fl, "%d|%d|%d|%.2f|%.2f|%s|%s\n",
                i, orderkey, partkey, price, discount, rf, ls);
    }
    fclose(fl);
    printf("Generated data/lineitem.tbl (50000 rows)\n");
    printf("Total records: 100000\n");
    return 0;
}
