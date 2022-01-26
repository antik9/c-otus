#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(char* msg) {
    puts(msg);
    exit(1);
}

char* cp1251_mapping[128] = {
    "Ђ", "Ѓ", "‚", "ѓ", "„", "…", "†", "‡", "€", "‰", "Љ", "‹", "Њ", "Ќ", "Ћ",
    "Џ", "ђ", "‘", "’", "“", "”", "•", "–", "—", " ", "™", "љ", "›", "њ", "ќ",
    "ћ", "џ", " ", "Ў", "ў", "Ј", "¤", "Ґ", "¦", "§", "Ё", "©", "Є", "«", "¬",
    "­", "®", "Ї", "°", "±", "І", "і", "ґ", "µ", "¶", "·", "ё", "№", "є", "»",
    "ј", "Ѕ", "ѕ", "ї", "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К",
    "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ",
    "Ъ", "Ы", "Ь", "Э", "Ю", "Я", "а", "б", "в", "г", "д", "е", "ж", "з", "и",
    "й", "к", "л", "м", "н", "о", "п", "р", "с", "т", "у", "ф", "х", "ц", "ч",
    "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я",
};

char* koi8r_mapping[128] = {
    "─", "│", "┌", "┐", "└", "┘", "├", "┤", "┬", "┴", "┼", "▀", "▄", "█", "▌",
    "▐", "░", "▒", "▓", "⌠", "■", "∙", "√", "≈", "≤", "≥", " ", "⌡", "°", "²",
    "·", "÷", "═", "║", "╒", "ё", "╓", "╔", "╕", "╖", "╗", "╘", "╙", "╚", "╛",
    "╜", "╝", "╞", "╟", "╠", "╡", "Ё", "╢", "╣", "╤", "╥", "╦", "╧", "╨", "╩",
    "╪", "╫", "╬", "©", "ю", "а", "б", "ц", "д", "е", "ф", "г", "х", "и", "й",
    "к", "л", "м", "н", "о", "п", "я", "р", "с", "т", "у", "ж", "в", "ь", "ы",
    "з", "ш", "э", "щ", "ч", "ъ", "Ю", "А", "Б", "Ц", "Д", "Е", "Ф", "Г", "Х",
    "И", "Й", "К", "Л", "М", "Н", "О", "П", "Я", "Р", "С", "Т", "У", "Ж", "В",
    "Ь", "Ы", "З", "Ш", "Э", "Щ", "Ч", "Ъ",
};

char* iso8859_5_mapping[96] = {
    " ", "Ё", "Ђ", "Ѓ", "Є", "Ѕ", "І", "Ї", "Ј", "Љ", "Њ", "Ћ", "Ќ", "—",
    "Ў", "Џ", "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л",
    "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ",
    "Ъ", "Ы", "Ь", "Э", "Ю", "Я", "а", "б", "в", "г", "д", "е", "ж", "з",
    "и", "й", "к", "л", "м", "н", "о", "п", "р", "с", "т", "у", "ф", "х",
    "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я", "№", "ё", "ђ", "ѓ",
    "є", "ѕ", "і", "ї", "ј", "љ", "њ", "ћ", "ќ", "ў", "џ"};

#define encode(source, dest, encoding) \
    _encode(source, dest, encoding,    \
            256 - (int)sizeof(encoding) / sizeof(encoding[0]))

void _encode(char* source, char* dest, char** encoding, int pivot) {
    FILE* in = fopen(source, "r");
    if (in == NULL) error("cannot open the source file");

    FILE* out = fopen(dest, "w");
    if (out == NULL) {
        fclose(in);
        error("cannot open the destination file");
    }

    bool ok = false;

    unsigned char next;
    while (fread(&next, 1, 1, in)) {
        if (next < pivot) {
            if (!fwrite(&next, 1, 1, out)) goto exit;
        } else {
            char* decoded = encoding[next - pivot];
            if (!(fwrite(decoded, strlen(decoded), 1, out))) goto exit;
        }
    }
    ok = true;

exit:
    if (!ok) puts("cannot write next symbol to a file");
    fclose(in);
    fclose(out);
}

int main(int argc, char* argv[]) {
    if (argc != 4)
        error("Usage:  utf-encoder <encoding> <source> <destination>");

    char* encoding = argv[1];
    char* source = argv[2];
    char* dest = argv[3];

    if (!strcmp("cp-1251", encoding)) {
        encode(source, dest, cp1251_mapping);
    } else if (!strcmp("koi8-r", encoding)) {
        encode(source, dest, koi8r_mapping);
    } else if (!strcmp("iso-8859-5", encoding)) {
        encode(source, dest, iso8859_5_mapping);
    } else {
        error("invalid encoding: expecting one of cp-1251, koi8-r, iso-8859-5");
    }

    return 0;
}
