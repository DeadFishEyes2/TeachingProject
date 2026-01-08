CC = gcc
CFLAGS = -Wall -g -Idev $(shell pkg-config --cflags libcurl)
LIBS = $(shell pkg-config --libs libcurl) -lm

DEV_DIR = dev
INPUT_DIR = input

# Găsim automat toate fișierele create_task_*.c
SOURCES = $(wildcard $(DEV_DIR)/create_task_*.c)
# Generăm numele executabilelor (fără path și extensie)
TARGETS = $(patsubst $(DEV_DIR)/%.c, %, $(SOURCES))

# Regula implicită
all: $(TARGETS)

# Regulă generică pentru a compila orice creator de task
create_task_%: $(DEV_DIR)/create_task_%.c $(DEV_DIR)/cJSON.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compilarea modulului cJSON
$(DEV_DIR)/cJSON.o: $(DEV_DIR)/cJSON.c $(DEV_DIR)/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DEV_DIR)/*.o $(TARGETS)