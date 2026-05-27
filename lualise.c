#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Include standard Lua headers
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Helper to trim leading/trailing whitespace
void trim(char *out, const char *in) {
    while (isspace((unsigned char)*in)) in++;
    size_t len = strlen(in);
    while (len > 0 && isspace((unsigned char)in[len - 1])) len--;
    strncpy(out, in, len);
    out[len] = '\0';
}

// ==========================================
// 1. THE ENGLISH TO LUA PARSER
// ==========================================
int parse_line(const char *line, char *lua_out) {
    char trimmed[256];
    trim(trimmed, line);

    // Skip empty lines or pseudo-comments
    if (strlen(trimmed) == 0 || trimmed[0] == '#') {
        lua_out[0] = '\0';
        return 1;
    }

    char var1[64], var2[64], val[128];

    // Rule 1: "Create a variable named X with value Y" -> "local X = Y"
    if (sscanf(trimmed, "Create a variable named %63s with value %127[^\n]", var1, val) == 2) {
        sprintf(lua_out, "local %s = %s", var1, val);
        return 1;
    }
    // Rule 2: "Set X to Y" -> "X = Y"
    if (sscanf(trimmed, "Set %63s to %127[^\n]", var1, val) == 2) {
        sprintf(lua_out, "%s = %s", var1, val);
        return 1;
    }
    // Rule 3: "If X is Y then" -> "if X == Y then"
    if (sscanf(trimmed, "If %63s is %63s then", var1, var2) == 2) {
        sprintf(lua_out, "if %s == %s then", var1, var2);
        return 1;
    }
    // Rule 4: "Otherwise, if X is Y then" -> "elseif X == Y then"
    if (sscanf(trimmed, "Otherwise, if %63s is %63s then", var1, var2) == 2) {
        sprintf(lua_out, "elseif %s == %s then", var1, var2);
        return 1;
    }
    // Rule 5: "Print "literal string"" -> "print("literal string")"
    if (strstr(trimmed, "Print \"") == trimmed) {
        char quote_val[128];
        if (sscanf(trimmed, "Print \"%127[^\"]\"", quote_val) == 1) {
            sprintf(lua_out, "print(\"%s\")", quote_val);
            return 1;
        }
    }
    // Rule 6: "Print X" (Variable) -> "print(X)"
    if (sscanf(trimmed, "Print %63s", var1) == 1) {
        sprintf(lua_out, "print(%s)", var1);
        return 1;
    }
    // Rule 7: "Otherwise" -> "else"
    if (strcmp(trimmed, "Otherwise") == 0) {
        strcpy(lua_out, "else");
        return 1;
    }
    // Rule 8: "End block" or "End of clause" -> "end"
    if (strcmp(trimmed, "End block") == 0 || strcmp(trimmed, "End of clause") == 0) {
        strcpy(lua_out, "end");
        return 1;
    }

    // If no rules matched
    return 0; 
}

// Iterates through lines of English code and builds the Lua payload string
int compile_english_to_lua(const char *english, char *lua_buffer) {
    char copy[2048];
    strncpy(copy, english, sizeof(copy));
    
    lua_buffer[0] = '\0';
    char *line = strtok(copy, "\n");
    char parsed_line[256];

    while (line != NULL) {
        if (!parse_line(line, parsed_line)) {
            printf("[Parser Error] Could not interpret: %s\n", line);
            return 0;
        }
        if (strlen(parsed_line) > 0) {
            strcat(lua_buffer, parsed_line);
            strcat(lua_buffer, "\n");
        }
        line = strtok(NULL, "\n");
    }
    return 1;
}

// ==========================================
// 2. RUNNING THE SCRIPT VIA EMBEDDED LUA
// ==========================================
int main() {
    const char *english_code = 
        "Create a variable named player_health with value 100\n"
        "Set player_health to 5\n"
        "\n"
        "If player_health is 0 then\n"
        "    Print \"You are dead!\"\n"
        "Otherwise, if player_health is 5 then\n"
        "    Print \"Critical health warning!\"\n"
        "Otherwise\n"
        "    Print \"You are safe.\"\n"
        "End block\n";

    printf("--- Pseudo-English Input ---\n%s", english_code);
    printf("----------------------------\n");

    char lua_code_buffer[4096];
    
    // Step 1: Parse/Compile
    if (!compile_english_to_lua(english_code, lua_code_buffer)) {
        return EXIT_FAILURE;
    }

    printf("\n--- Compiled Lua Output ---\n%s", lua_code_buffer);
    printf("----------------------------\n");

    // Step 2: Initialize embedded Lua VM state
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        fprintf(stderr, "Failed to initialize Lua state.\n");
        return EXIT_FAILURE;
    }
    
    luaL_openlibs(L); // Open standard libraries (like print)

    // Step 3: Run the compiled Lua chunk
    printf("\n--- Running Lua Execution Output ---\n");
    if (luaL_dostring(L, lua_code_buffer) != LUA_OK) {
        const char *error = lua_tostring(L, -1);
        fprintf(stderr, "Lua Execution Error: %s\n", error);
        lua_close(L);
        return EXIT_FAILURE;
    }
    printf("------------------------------------\n");

    // Cleanup
    lua_close(L);
    return EXIT_SUCCESS;
}
