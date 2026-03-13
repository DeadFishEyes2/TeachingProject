CC = gcc
CFLAGS = -Wall -g -Idev $(shell pkg-config --cflags libcurl)
LIBS = $(shell pkg-config --libs libcurl) -lm

DEV_DIR = dev
INPUT_DIR = input
SOLVE_DIR = solve

# Găsim automat toate fișierele create_task_*.c
SOURCES = $(wildcard $(DEV_DIR)/create_task_*.c)
# Generăm numele executabilelor (fără path și extensie)
TARGETS = $(patsubst $(DEV_DIR)/%.c, %, $(SOURCES))

# Găsim automat toate fișierele task_*.c din solve/
SOLVE_SOURCES = $(wildcard $(SOLVE_DIR)/task_*.c)
SOLVE_TARGETS = $(patsubst $(SOLVE_DIR)/%.c, %, $(SOLVE_SOURCES))

# Regula implicită
all: $(TARGETS) $(SOLVE_TARGETS)

# Regulă generică pentru a compila orice creator de task
create_task_%: $(DEV_DIR)/create_task_%.c $(DEV_DIR)/cJSON.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Regulă generică pentru a compila orice solver de task
task_%: $(SOLVE_DIR)/task_%.c $(DEV_DIR)/cJSON.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compilarea modulului cJSON
$(DEV_DIR)/cJSON.o: $(DEV_DIR)/cJSON.c $(DEV_DIR)/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DEV_DIR)/*.o $(TARGETS) $(SOLVE_TARGETS)