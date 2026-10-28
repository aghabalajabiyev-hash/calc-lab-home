// Aghabala Jabiyev //251ADB102
// https://github.com/aghabalajabiyev-hash/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: course assignment instructions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct
{
    const char *text;
    size_t pos;
    size_t error_pos;
    int has_error;
} Parser;

typedef struct
{
    double value;
    int is_float;
} Value;
static Value parse_expression(Parser *p);
// Spaces and tabs separate tokens but do not change the expression's meaning
static void skip_spaces(Parser *p)
{
    while (p->text[p->pos] == ' ' ||
           p->text[p->pos] == '\t')
    {
        p->pos++;
    }
}

static void set_error(Parser *p, size_t position)
{
    if (!p->has_error)
    {
        p->has_error = 1;
        p->error_pos = position;
    }
}

static Value make_value(double number, int is_float)
{
    Value result;
    result.value = number;
    result.is_float = is_float;
    return result;
}

static Value parse_number(Parser *p)
{
    skip_spaces(p);

    size_t start = p->pos;
    double number = 0.0;
    int is_float = 0;

    if (!isdigit((unsigned char)p->text[p->pos]))
    {
        set_error(p, p->pos + 1);
        return make_value(0.0, 0);
    }

    while (isdigit((unsigned char)p->text[p->pos]))
    {
        number = number * 10.0 + (p->text[p->pos] - '0');
        p->pos++;
    }

    if (p->text[p->pos] == '.')
    {
        is_float = 1;
        p->pos++;

        if (!isdigit((unsigned char)p->text[p->pos]))
        {
            set_error(p, p->pos);
            return make_value(0.0, 1);
        }

        double place = 0.1;
        while (isdigit((unsigned char)p->text[p->pos]))
        {
            number += (p->text[p->pos] - '0') * place;
            place *= 0.1;
            p->pos++;
        }
    }

    (void)start;
    return make_value(number, is_float);
}

static Value parse_primary(Parser *p)
{
    skip_spaces(p);

    if (p->text[p->pos] == '(')
    {
        p->pos++;

        Value result = parse_expression(p);

        skip_spaces(p);

        if (p->text[p->pos] == ')')
        {
            p->pos++;
        }
        else
        {
            set_error(p, p->pos + 1);
        }

        return result;
    }

    return parse_number(p);
}
static Value parse_power(Parser *p);
static Value parse_term(Parser *p)
{
    Value left = parse_power(p);

    while (!p->has_error)
    {
        skip_spaces(p);

        char op = p->text[p->pos];

        if (op != '*' && op != '/')
        {
            break;
        }

        if (op == '*' && p->text[p->pos + 1] == '*')
        {
            break;
        }

        p->pos++;

        skip_spaces(p);
        size_t right_start = p->pos;
        Value right = parse_power(p);

        if (p->has_error)
        {
            break;
        }

        if (op == '/')
        {
            if (right.value == 0.0)
            {
                set_error(p, right_start + 1);
                break;
            }

            if (!left.is_float && !right.is_float)
            {
                left.value = (long long)left.value /
                             (long long)right.value;
            }
            else
            {
                left.value /= right.value;
            }

            left.is_float = left.is_float || right.is_float;
        }
        else
        {
            left.value *= right.value;
            left.is_float = left.is_float || right.is_float;
        }
    }

    return left;
}

static Value parse_power(Parser *p)
{
    Value left = parse_primary(p);

    if (p->has_error)
    {
        return left;
    }

    skip_spaces(p);

    if (p->text[p->pos] == '*' &&
        p->text[p->pos + 1] == '*')
    {

        size_t operator_start = p->pos;
        p->pos += 2;
        skip_spaces(p);
        size_t exponent_start = p->pos;

        Value right = parse_power(p);

        if (p->has_error)
        {
            return left;
        }

        if (!left.is_float && !right.is_float &&
            right.value < 0)
        {
            set_error(p, exponent_start + 1);
            return left;
        }

        double result = pow(left.value, right.value);

        if (!isfinite(result))
        {
            set_error(p, operator_start + 1);
            return left;
        }

        left.value = result;
        left.is_float = left.is_float || right.is_float;
    }

    return left;
}
// Calculates addition and subtraction.
static Value parse_expression(Parser *p)
{
    Value left = parse_term(p);

    while (!p->has_error)
    {
        skip_spaces(p);

        char op = p->text[p->pos];

        if (op != '+' && op != '-')
        {
            break;
        }

        p->pos++;

        Value right = parse_term(p);

        if (p->has_error)
        {
            break;
        }

        if (op == '+')
        {
            left.value += right.value;
        }
        else
        {
            left.value -= right.value;
        }

        left.is_float = left.is_float || right.is_float;
    }

    return left;
}
int main(int argc, char *argv[])
{
    const char *input_name = NULL;
    const char *output_dir = NULL;
    char default_dir[512];
    char output_path[1024];

    if (argc == 4 && strcmp(argv[1], "-o") == 0)
    {
        output_dir = argv[2];
        input_name = argv[3];
    }
    else if (argc == 2)
    {
        input_name = argv[1];

        const char *base = strrchr(input_name, '/');
        base = (base == NULL) ? input_name : base + 1;

        size_t len = strlen(base);
        if (len >= sizeof(default_dir) - 1)
        {
            fprintf(stderr, "Error: filename too long\n");
            return 1;
        }

        strcpy(default_dir, base);
        char *dot = strrchr(default_dir, '.');
        if (dot != NULL && strcmp(dot, ".txt") == 0)
        {
            *dot = '\0';
        }

        if (snprintf(output_path, sizeof(output_path),
                     "%s_aghabala_jabiyev_251ADB102",
                     default_dir) >= (int)sizeof(output_path))
        {
            fprintf(stderr, "Error: output path too long\n");
            return 1;
        }

        strcpy(default_dir, output_path);
        output_dir = default_dir;
    }
    else
    {
        fprintf(stderr, "Usage: ./calc [-o OUTDIR] input.txt\n");
        return 1;
    }

    FILE *input_file = fopen(input_name, "r");
    if (input_file == NULL)
    {
        fprintf(stderr, "Error: cannot open input file\n");
        return 1;
    }

    struct stat st;
    if (stat(output_dir, &st) != 0)
    {
        if (mkdir(output_dir, 0777) != 0)
        {
            fprintf(stderr, "Error: cannot create output directory\n");
            fclose(input_file);
            return 1;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Error: output path is not a directory\n");
        fclose(input_file);
        return 1;
    }

    const char *base = strrchr(input_name, '/');
    base = (base == NULL) ? input_name : base + 1;

    char input_base[512];
    if (strlen(base) >= sizeof(input_base))
    {
        fprintf(stderr, "Error: filename too long\n");
        fclose(input_file);
        return 1;
    }

    strcpy(input_base, base);
    char *dot = strrchr(input_base, '.');
    if (dot != NULL && strcmp(dot, ".txt") == 0)
    {
        *dot = '\0';
    }

    if (snprintf(output_path, sizeof(output_path),
                 "%s/%s_aghabala_jabiyev_251ADB102.txt",
                 output_dir, input_base) >= (int)sizeof(output_path))
    {
        fprintf(stderr, "Error: output path too long\n");
        fclose(input_file);
        return 1;
    }

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL)
    {
        fprintf(stderr, "Error: cannot create output file\n");
        fclose(input_file);
        return 1;
    }

    if (fseek(input_file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Error: cannot read input file\n");
        fclose(input_file);
        fclose(output_file);
        return 1;
    }

    long file_size = ftell(input_file);
    if (file_size < 0 ||
        fseek(input_file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error: cannot read input file\n");
        fclose(input_file);
        fclose(output_file);
        return 1;
    }

    char *contents = malloc((size_t)file_size + 1);
    if (contents == NULL)
    {
        fprintf(stderr, "Error: not enough memory\n");
        fclose(input_file);
        fclose(output_file);
        return 1;
    }

    size_t file_length = fread(contents, 1, (size_t)file_size,
                               input_file);
    if (file_length != (size_t)file_size)
    {
        fprintf(stderr, "Error: cannot read input file\n");
        free(contents);
        fclose(input_file);
        fclose(output_file);
        return 1;
    }
    contents[file_length] = '\0';

    FILE *temporary = tmpfile();
    if (temporary == NULL)
    {
        fprintf(stderr, "Error: cannot create temporary file\n");
        free(contents);
        fclose(input_file);
        fclose(output_file);
        return 1;
    }

    size_t offset = 0;
    size_t global_error = 0;

    while (offset < file_length)
    {
        size_t line_start = offset;
        char *line = contents + offset;

        while (offset < file_length && contents[offset] != '\n')
        {
            offset++;
        }

        size_t line_length = offset - line_start;
        int has_newline = (offset < file_length);

        if (has_newline)
        {
            contents[offset] = '\0';
            offset++;
        }

        if (line_length > 0 && line[line_length - 1] == '\r')
        {
            line[line_length - 1] = '\0';
        }

        char *first = line;
        while (*first == ' ' || *first == '\t')
        {
            first++;
        }

        if (*first != '\0' && *first != '#')
        {
            Parser parser = {line, 0, 0, 0};
            Value result = parse_expression(&parser);
            skip_spaces(&parser);

            if (parser.has_error || line[parser.pos] != '\0')
            {
                size_t local_error;

                if (parser.has_error)
                {
                    local_error = parser.error_pos;
                }
                else
                {
                    local_error = parser.pos + 1;
                }
                global_error = line_start + local_error;
                break;
            }

            if (result.is_float)
            {
                fprintf(temporary, "%.15g\n", result.value);
            }
            else
            {
                fprintf(temporary, "%.0f\n", result.value);
            }
        }

        if (has_newline)
        {
            contents[offset - 1] = '\n';
        }
    }

    if (global_error != 0)
    {
        fprintf(output_file, "ERROR:%zu\n", global_error);
    }
    else
    {
        rewind(temporary);
        int ch;
        while ((ch = fgetc(temporary)) != EOF)
        {
            fputc(ch, output_file);
        }
    }

    fclose(temporary);
    free(contents);

    fclose(input_file);
    fclose(output_file);
    return 0;
}