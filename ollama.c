#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void json_escape(const char *input, char *output, size_t output_size) {
    size_t j = 0;

    for (size_t i = 0; input[i] != '\0' && j + 2 < output_size; i++) {
        char c = input[i];

        if (c == '"' || c == '\\') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = c;
        } else if (c == '\n') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 'n';
        } else if (c == '\r') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 'r';
        } else if (c == '\t') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 't';
        } else {
            output[j++] = c;
        }
    }

    output[j] = '\0';
}

static int parse_ollama_move(const char *response) {
    char *start = strstr(response, "\"response\":\"");
    if (start == NULL) {
        return -1;
    }

    start += strlen("\"response\":\"");

    for (int i = 0; start[i] != '\0' && start[i] != '"'; i++) {
        if (start[i] >= '0' && start[i] <= '6') {
            return start[i] - '0';
        }
    }

    return -1;
}

static int parse_text_move(const char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] >= '0' && text[i] <= '6') {
            return text[i] - '0';
        }
    }

    return -1;
}

int ask_ollama_for_move(const char *prompt) {
    char escaped_prompt[4096];
    char json_data[8192];
    char filename[128];
    char command[512];
    char response[8192];

    json_escape(prompt, escaped_prompt, sizeof(escaped_prompt));

    snprintf(json_data, sizeof(json_data),
        "{\"model\":\"gemma4:e2b-mlx\",\"prompt\":\"%s\",\"stream\":false}",
        escaped_prompt
    );

    snprintf(filename, sizeof(filename), "/tmp/ollama_request_%d.json", getpid());

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }

    fputs(json_data, file);
    fclose(file);

    snprintf(command, sizeof(command),
        "curl -s --max-time 30 http://localhost:11434/api/generate "
        "-H 'Content-Type: application/json' "
        "-d @%s",
        filename
    );

    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        remove(filename);
        return -1;
    }

    size_t length = fread(response, 1, sizeof(response) - 1, pipe);
    response[length] = '\0';

    pclose(pipe);
    remove(filename);

    return parse_ollama_move(response);
}

int ask_hermes_for_move(const char *prompt) {
    char filename[128];
    char command[512];
    char response[8192];

    snprintf(filename, sizeof(filename), "/tmp/hermes_prompt_%d.txt", getpid());

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }

    fputs(prompt, file);
    fclose(file);

    snprintf(command, sizeof(command),
        "hermes -z \"$(cat %s)\"",
        filename
    );

    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        remove(filename);
        return -1;
    }

    size_t length = fread(response, 1, sizeof(response) - 1, pipe);
    response[length] = '\0';

    pclose(pipe);
    remove(filename);

    return parse_text_move(response);
}
