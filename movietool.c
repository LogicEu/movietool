#include <xlsxwriter/xlsxwriter.h>
#include <pdftool/pdftool.h>
#include <utopia/Utopia.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUFF_SIZE 1024

static char* moments[] = {
    "Indefinido",
    "Día",
    "Noche"
};

static char* places[] = {
    "Indefinido",
    "Interior",
    "Exterior",
    "Interior / Exterior"
};

typedef enum SpaceEnum {
    SPACE_NOT_FOUND,
    SPACE_INT,
    SPACE_EXT,
    SPACE_INT_EXT
} SpaceEnum;

typedef enum TimeEnum {
    TIME_NOT_FOUND,
    TIME_DAY,
    TIME_NIGHT
} TimeEnum;

typedef struct Context {
    char location[256];
    TimeEnum time;
    SpaceEnum place;
} Context;

typedef struct Character {
    char name[256];
} Character;

typedef struct Scene {
    unsigned int scene_index, page_start, page_end, closed;
    Context context;
    array_t* characters;
} Scene;

static Character characterCreate(const char* name)
{
    Character c;
    strcpy(c.name, name);
    return c;
}

static Scene sceneOpen(Context context, unsigned int scene_index, unsigned int page_start)
{
    Scene s;
    s.closed = 0;
    s.context = context;
    s.scene_index = scene_index;
    s.page_start = page_start;
    s.characters = array_new(1, sizeof(unsigned int));
    return s;
}

static void sceneClose(Scene* scene, unsigned int page_end)
{
    scene->closed = 1;
    scene->page_end = page_end;
}

static bool haschar(char c, const char* keyword)
{
    size_t size = strlen(keyword);
    for (size_t i = 0; i < size; i++) {
        if (keyword[i] == c) return true;
    }
    return false;
}

static void line_erase_char(char* line, size_t size, size_t index)
{
    size_t i;
    for (i = index; i < size - 1; i++) {
        line[i] = line[i + 1];
    }
    line[i] = '\0';
    return;
}

static void fix_line(char* line)
{
    size_t size = strlen(line);
    for (size_t i = 0; i < size; i++) {
        if ((int)line[i] < 32 || (int)line[i] > 126) line_erase_char(line, size, i);
    }
}

static SpaceEnum scene_header_space(char* line, size_t size)
{
    if (sub_string_find(line, "INT/EXT", size) != -1 ||
        sub_string_find(line, "EXT/INT", size) != -1) {
        return SPACE_INT_EXT;
    } 
    if (sub_string_find(line, "INT", size) != -1) return SPACE_INT;
    if (sub_string_find(line, "EXT", size) != -1) return SPACE_EXT;
    return SPACE_NOT_FOUND;
}

static TimeEnum scene_header_time(char* line, size_t size)
{
    if (sub_string_find(line, "DAY", size) != -1 ||
        sub_string_find(line, "DÍA", size) != -1 ||
        sub_string_find(line, "DIA", size) != -1 ||
        sub_string_find(line, "DA", size) != -1) {
        return TIME_DAY;
    } 
    
    if (sub_string_find(line, "NIGHT", size) != -1 ||
        sub_string_find(line, "NOCHE", size) != -1) {
        return TIME_NIGHT;
    }
    return TIME_NOT_FOUND;
}

static Context parse_scene_header(const char* line, size_t size)
{
    Context c = { "\0", TIME_NOT_FOUND, SPACE_NOT_FOUND };
    char word[256];
    size_t marker = 0;
    while (1) {
        bool found = false;
        int rc = sscanf(&line[marker], "%s", word);
        if (rc == EOF) break;
        //printf("%s\n", word);
        if (c.place == SPACE_NOT_FOUND) {
            SpaceEnum se = scene_header_space(word, size);
            if (se != SPACE_NOT_FOUND) {
                c.place = se;
                found = true;
            }
        }
        if (c.time == TIME_NOT_FOUND) {
            TimeEnum te = scene_header_time(word, size);
            if (te != TIME_NOT_FOUND) {
                c.time = te;
                found = true;
            }
        }
        if (!found && !haschar('-', word)) {
            if (strlen(word) + strlen(c.location) + 1 < 256) {
                strcat(c.location, " ");
                strcat(c.location, word);
            }
        }

        while(1) {
            char ch = line[marker++];
            if (ch == ' ' || ch == '\0' || ch == '\n') break;
        }
    }
    return c;
}

static void array_conditional_push(array_t* array, void* data)
{
    for (unsigned int i = 0; i < array->used; i++) {
        if (!memcmp(data, array_index(array, i), array->bytes)) {
            return;
        }
    }
    array_push(array, data);
}

static void line_parse_keywords(char* line, Scene* scene, array_t* keywords)
{
    unsigned int k = 0;
    size_t size = strlen(line);
    Character* ch = (Character*)keywords->data;
    for (Character* end = ch + keywords->used; ch != end; ch++) {
        if (sub_string_find(line, ch->name, size) != -1) {
            array_conditional_push(scene->characters, &k);
        }
        k++;
    }
}

static array_t* script_report(const char* path, array_t* keywords)
{
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Could not open file '%s'\n", path);
        return NULL;
    }

    array_t* scenes = array_new(16, sizeof(Scene));
    unsigned int page_mark = 1, scene_mark = 0;
    Scene s;
    s.closed = 1;
    Context context;
    bool foundContext = false, foundNum = false;

    char line[BUFF_SIZE];
    while (fgets(line, BUFF_SIZE, file)) {
        //Page and Scene Count
        fix_line(line);
        unsigned int size = strlen(line);
        if (isdigit(line[0]) && !haschar(' ', line)) {
            if (haschar('.', line)) {
                sscanf(line, "%u.", &page_mark);
            } else if (!foundNum) {
                unsigned int num;
                sscanf(line, "%u", &num);
                if (num == scene_mark + 1) {
                    scene_mark = num;
                    foundNum = true;
                }
                printf("%s - %d\n", line, scene_mark);
            }
            printf("Page: %u - Scene: %u\n%s", page_mark, scene_mark, line);
        }
        //Scene Header 
        if (sub_string_find(line, "INT", size) != -1 ||
            sub_string_find(line, "EXT", size) != -1) {
            if (!s.closed) {
                sceneClose(&s, page_mark);
                array_push(scenes, &s);
            }
            context = parse_scene_header(line, size);
            foundContext = true;
        }
        if (foundContext && foundNum) {
            s = sceneOpen(context, scene_mark, page_mark);
            foundContext = false;
            foundNum = false;
        }
        if (!s.closed) line_parse_keywords(line, &s, keywords);
    }
    fclose(file);
    return scenes;
}

static void print_context(Context* c)
{
    printf("Locación: %s\n", c->location);
    
    char p[128];
    if (c->time == TIME_DAY) strcpy(p, "Momento: Día");
    else if (c->time == TIME_NIGHT) strcpy(p, "Momento: Noche");
    else strcpy(p, "Momento: Indefinido");
    printf("%s\n", p);

    if (c->place == SPACE_INT_EXT) strcpy(p, "Espacio: Interior / Exterior");
    else if (c->place == SPACE_INT) strcpy(p, "Espacio: Interior");
    else if (c->place == SPACE_EXT) strcpy(p, "Espacio: Exterior");
    else strcpy(p, "Espacio: Indefinido");
    printf("%s\n", p);
}

static void print_scene(Scene* s, array_t* characters)
{
    printf("ESCENA %u\n", s->scene_index);
    printf("Páginas: %u - %u\n", s->page_start, s->page_end);
    print_context(&s->context);

    printf("Personajes: ");
    unsigned int* k = (unsigned int*)s->characters->data;
    for (unsigned int* end = k + s->characters->used; k != end; k++) {
        Character* ch = (Character*)array_index(characters, *k);
        printf("%s ", ch->name);
    }
    printf("\n********************************\n");
}

static void print_scenes(array_t* scenes, array_t* characters)
{
    Scene* s = (Scene*)scenes->data;
    for (Scene* end = s + scenes->used; s != end; s++) {
        print_scene(s, characters);
    }
}

static void print_excel_scenes(array_t* scenes, array_t* characters)
{
    lxw_workbook *workbook = workbook_new("desglose.xlsx");
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

    lxw_format *bold = workbook_add_format(workbook);
    format_set_bold(bold);
 
    /* Start from the first cell. Rows and columns are zero indexed. */
    int row = 0, col = 0;
    worksheet_write_string(worksheet, row, col++, "ESCENA", bold);
    worksheet_write_string(worksheet, row, col++, "Páginas", bold);
    worksheet_write_string(worksheet, row, col++, "Locación", bold);
    worksheet_write_string(worksheet, row, col++, "Momento", bold);
    worksheet_write_string(worksheet, row, col++, "Espacio", bold);
    worksheet_write_string(worksheet, row, col++, "Personajes", bold);
    
    Scene* s = scenes->data;
    char temp[1024];
    for (row = 1; row < scenes->used + 1; row++) {
        col = 0;

        worksheet_write_number(worksheet, row, col++, s->scene_index, NULL);
        
        sprintf(temp, "(%u - %u)", s->page_start, s->page_end);
        worksheet_write_string(worksheet, row, col++, temp, NULL);
        
        worksheet_set_column(worksheet, col, col, 45, NULL);
        worksheet_write_string(worksheet, row, col++, s->context.location, NULL);
        
        worksheet_write_string(worksheet, row, col++, moments[s->context.time], NULL);
        worksheet_set_column(worksheet, col, col, 15, NULL);
        worksheet_write_string(worksheet, row, col++, places[s->context.place], NULL);

        bzero(temp, 1024);
        unsigned int* k = (unsigned int*)s->characters->data;
        for (unsigned int i = 0; i < s->characters->used; i++) {
            Character* ch = (Character*)array_index(characters, *k);
            strcat(temp, ch->name);
            strcat(temp, " ");
            k++;
        }
        printf("%s\n", temp);
        worksheet_set_column(worksheet, col, col, 40, NULL);
        worksheet_write_string(worksheet, row, col++, temp, NULL);
        s++;
    }
 
    /* Save the workbook and free any allocated memory. */
    lxw_error error = workbook_close(workbook);
 
    /* Check if there was any error creating the xlsx file. */
    if (error) printf("Error in workbook_close().\n"
               "Error %d = %s\n", error, lxw_strerror(error));
}

int main(int argc, char** argv)
{
    char in_pdf[256];
    array_t* keywords = array_new(1, sizeof(Character));

    if (argc < 2) {
        printf("Enter input PDF file.\n");
        return 0;
    } 
    strcpy(in_pdf, argv[1]);
    for (int i = 2; i < argc; i++) {
        fix_line(argv[i]);
        Character ch = characterCreate(argv[i]);
        array_push(keywords, &ch);
    }

    pdf_to_txt_file(in_pdf, "tmp.txt");
    array_t* scenes = script_report("tmp.txt", keywords);
    print_scenes(scenes, keywords);
    print_excel_scenes(scenes, keywords);
    //system("cat tmp.txt");
    
    array_destroy(keywords);
    array_destroy(scenes);
    //system("rm tmp.txt");
    return 0;
}
