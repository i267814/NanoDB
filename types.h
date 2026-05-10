#pragma once
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ─── Base Field (Polymorphic) ─────────────────────────────────────────────────
struct Field {
    virtual ~Field() {}
    virtual int    compare(const Field* other) const = 0;
    virtual Field* clone()                    const = 0;
    virtual void   print()                    const = 0;
    virtual void   serialize(char* buf, int& offset) const = 0;
    virtual void   deserialize(const char* buf, int& offset) = 0;
    virtual int    typeId() const = 0; // 0=int, 1=float, 2=string

    bool operator==(const Field& o) const { return compare(&o) == 0; }
    bool operator!=(const Field& o) const { return compare(&o) != 0; }
    bool operator< (const Field& o) const { return compare(&o) <  0; }
    bool operator> (const Field& o) const { return compare(&o) >  0; }
    bool operator<=(const Field& o) const { return compare(&o) <= 0; }
    bool operator>=(const Field& o) const { return compare(&o) >= 0; }
};

// ─── IntField ─────────────────────────────────────────────────────────────────
struct IntField : Field {
    int value;
    IntField(int v = 0) : value(v) {}
    int typeId() const override { return 0; }
    int compare(const Field* o) const override {
        const IntField* oi = static_cast<const IntField*>(o);
        return (value > oi->value) - (value < oi->value);
    }
    Field* clone() const override { return new IntField(value); }
    void   print() const override { printf("%d", value); }
    void serialize(char* buf, int& off) const override {
        memcpy(buf + off, &value, sizeof(int)); off += sizeof(int);
    }
    void deserialize(const char* buf, int& off) override {
        memcpy(&value, buf + off, sizeof(int)); off += sizeof(int);
    }
};

// ─── FloatField ───────────────────────────────────────────────────────────────
struct FloatField : Field {
    float value;
    FloatField(float v = 0.0f) : value(v) {}
    int typeId() const override { return 1; }
    int compare(const Field* o) const override {
        const FloatField* of = static_cast<const FloatField*>(o);
        if (value < of->value) return -1;
        if (value > of->value) return  1;
        return 0;
    }
    Field* clone() const override { return new FloatField(value); }
    void   print() const override { printf("%.2f", value); }
    void serialize(char* buf, int& off) const override {
        memcpy(buf + off, &value, sizeof(float)); off += sizeof(float);
    }
    void deserialize(const char* buf, int& off) override {
        memcpy(&value, buf + off, sizeof(float)); off += sizeof(float);
    }
};

// ─── StringField ──────────────────────────────────────────────────────────────
static const int MAX_STR = 64;
struct StringField : Field {
    char value[MAX_STR];
    StringField() { value[0] = '\0'; }
    StringField(const char* v) { strncpy(value, v, MAX_STR - 1); value[MAX_STR-1]='\0'; }
    int typeId() const override { return 2; }
    int compare(const Field* o) const override {
        const StringField* os = static_cast<const StringField*>(o);
        return strncmp(value, os->value, MAX_STR);
    }
    Field* clone() const override { return new StringField(value); }
    void   print() const override { printf("%s", value); }
    void serialize(char* buf, int& off) const override {
        memcpy(buf + off, value, MAX_STR); off += MAX_STR;
    }
    void deserialize(const char* buf, int& off) override {
        memcpy(value, buf + off, MAX_STR); off += MAX_STR;
    }
};

// ─── Row ─────────────────────────────────────────────────────────────────────
static const int MAX_FIELDS = 16;
struct Row {
    Field* fields[MAX_FIELDS];
    int    numFields;

    Row() : numFields(0) { for (int i=0;i<MAX_FIELDS;i++) fields[i]=nullptr; }
    ~Row() { clear(); }

    void clear() {
        for (int i = 0; i < numFields; i++) {
            delete fields[i]; fields[i] = nullptr;
        }
        numFields = 0;
    }

    void addField(Field* f) {
        if (numFields < MAX_FIELDS) fields[numFields++] = f;
    }

    Row* clone() const {
        Row* r = new Row();
        for (int i = 0; i < numFields; i++) r->addField(fields[i]->clone());
        return r;
    }

    void print() const {
        printf("(");
        for (int i = 0; i < numFields; i++) {
            fields[i]->print();
            if (i < numFields-1) printf(", ");
        }
        printf(")");
    }

    void serialize(char* buf, int& off) const {
        memcpy(buf+off, &numFields, sizeof(int)); off += sizeof(int);
        for (int i = 0; i < numFields; i++) {
            int tid = fields[i]->typeId();
            memcpy(buf+off, &tid, sizeof(int)); off += sizeof(int);
            fields[i]->serialize(buf, off);
        }
    }

    void deserialize(const char* buf, int& off) {
        clear();
        memcpy(&numFields, buf+off, sizeof(int)); off += sizeof(int);
        for (int i = 0; i < numFields; i++) {
            int tid;
            memcpy(&tid, buf+off, sizeof(int)); off += sizeof(int);
            if      (tid == 0) fields[i] = new IntField();
            else if (tid == 1) fields[i] = new FloatField();
            else               fields[i] = new StringField();
            fields[i]->deserialize(buf, off);
        }
    }
};
