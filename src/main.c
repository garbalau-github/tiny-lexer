#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// File size
size_t file_size(FILE *file)
{
  if (!file)
  {
    return 0;
  }
  fpos_t original;
  if (fgetpos(file, &original) != 0)
  {
    printf("fgetpos() failed");
  }

  fseek(file, 0, SEEK_END);
  long out = ftell(file);

  if (fsetpos(file, &original) != 0)
  {
    printf("fgetpos() failed");
  }
  return out;
}

// File contents (as heap allocated buffer)
char *file_contents(char *path)
{
  FILE *file = fopen(path, "r");
  if (!file)
  {
    printf("Error opening file: %s\n", path);
    return NULL;
  }
  // Buffer for correct size + null terminator (for strings)
  long size = file_size(file);
  char *contents = malloc(size + 1);
  char *write_it = contents;

  size_t bytes_read = 0;
  size_t bytes_read_this_iteration = 0;

  while (bytes_read < size)
  {
    printf("Reading %ld bytes\n", size - bytes_read);
    // Read file into buffer
    size_t bytes_read_this_iteration = fread(write_it, 1, size - bytes_read, file);
    if (ferror(file))
    {
      printf("Error reading file: %s\n", path);
      free(contents);
      return NULL;
    }

    bytes_read += bytes_read_this_iteration;
    write_it += bytes_read_this_iteration;

    if (feof(file))
    {
      break;
    }
  }

  contents[bytes_read] = '\0';
  return contents;
}

void print_usage(char **argv)
{
  printf("USAGE: %s <path_to_file_to_compile>\n", argv[0]);
}

typedef struct Error
{
  enum ErrorType
  {
    ERROR_NONE = 0,
    ERROR_ARGUMENTS,
    ERROR_TYPE,
    ERROR_GENERIC,
    ERROR_SYNTAX,
    ERROR_TODO,
    // ERROR_FILE,
    ERROR_MAX
  } type;
  char *msg;
} Error;

Error ok = {ERROR_NONE, NULL};

void print_error(Error err)
{
  if (err.type == ERROR_NONE)
  {
    return;
  }
  printf("ERROR: %s\n", err.msg);
  assert(ERROR_MAX == 6);
  switch (err.type)
  {
  default:
    printf("Unknown error type...");
    break;
  case ERROR_TODO:
    printf("TODO (not implemented)");
    break;
  case ERROR_SYNTAX:
    printf("Invalid syntax");
    break;
  case ERROR_TYPE:
    printf("Mismatched types");
    break;
  case ERROR_ARGUMENTS:
    printf("Invalid arguments");
    break;
  case ERROR_GENERIC:
    break;
  case ERROR_NONE:
    break;
  }
  putchar('\n');
  if (err.msg)
  {
    printf("Message: %s\n", err.msg);
  }
}

#define ERROR_CREATE(n, t, msg) Error(n) = {(t), (msg)};

#define ERROR_PREP(n, t, message) (n).type = (t), (n).msg = (message);

const char *whitespace = " \r\n";
const char *delimeters = " \r\n,():";

// Linked List
typedef struct Token
{
  char *beginning;
  char *end;
  struct Token *next;
} Token;

Token *token_create(void)
{
  // Allocate memory on the heap
  Token *token = malloc(sizeof(Token));
  // Assert that it exists
  assert(token && "Failed to allocate memory for token");
  // Set the memory to 0
  memset(token, 0, sizeof(Token));
  // Return our pointer
  return token;
}

void free_tokens(Token *root)
{
  while (root)
  {
    Token *token_to_free = root;
    root = root->next;
    free(token_to_free);
  }
}

void print_tokens(Token *root)
{
  size_t i = 0;
  while (root)
  {
    // little protection
    if (i > 10000)
    {
      break;
    }
    printf("Token %zu: ", i);
    if (root->beginning && root->end)
    {
      printf("%.*s", (int)(root->end - root->beginning), root->beginning);
    }
    putchar('\n');
    root = root->next;
    i++;
  }
}

// Lex the next current_token from the source and point beg and end to the start and end of the current_token
Error lex(char *source, Token *current_token)
{
  Error err = ok;
  if (!source || !current_token)
  {
    ERROR_PREP(err, ERROR_ARGUMENTS, "Cannot lex NULL source");
    return err;
  };

  current_token->beginning = source;
  // Skip whitespace in the beginning
  current_token->beginning += strspn(current_token->beginning, whitespace);
  // printf("Skipped whitespace: \n");
  current_token->end = current_token->beginning;

  if (*current_token->end == '\0')
  {
    return err;
  }

  // Skip everything which is not in delimeters
  current_token->end += strcspn(current_token->beginning, delimeters);

  if (current_token->end == current_token->beginning)
  {
    current_token->end += 1;
  }

  return ok;
}

// TOOD: API to create new node
// -- API to add node as child
typedef long long integer_t;
typedef struct Node
{
  enum NodeType
  {
    NODE_TYPE_NONE,
    NODE_TYPE_PROGRAM,
    NODE_TYPE_INTEGER,
    NODE_TYPE_MAX
  } type;

  union NodeValue
  {
    integer_t integer;
  } value;

  struct Node *children[3];
} Node;

// Functions returns boolean? Macro?
#define nonep(node) ((node).type == NODE_TYPE_NONE)
#define integerp(node) ((node).type == NODE_TYPE_INTEGER)

typedef struct Program
{
  Node *root;
} Program;

// Todo:
// - API to create a new bindin
// - API to add binding to env
typedef struct Binding
{
  char *id;
  Node *value;
  struct Binding *next;
} Binding;

// Todo:
// - API to create a new environment
typedef struct Environment
{
  struct Environment *parent;
  Binding *bind;
} Environment;

void environment_set()
{
}

// int @return Zero upon success
int valid_identifier(char *id)
{
  return strpbrk(id, delimeters) == NULL ? 1 : 0;
}

// @return boolean like value; 1 for success, 0 for failure
int token_string_equalp(char *string, Token *token)
{
  if (!string || !token)
  {
    return 0;
  }

  char *beg = token->beginning;
  while (*string && token->beginning < token->end)
  {
    if (*string != *beg)
    {
      return 0;
    }
    string++;
    beg++;
  }
  return 1;
}

Error parse_expr(char *source, Node *result)
{
  Token *tokens = NULL;
  Token *token_it = tokens;
  Token current_token;
  current_token.next = NULL;
  current_token.beginning = source;
  current_token.end = source;
  // char *beg = source;
  // char *end = source;
  Error err = ok;
  // While there is no error (we asign err to return value of lex
  // Then we get the type of error we asign to, and continue if it is none
  while ((err = lex(current_token.end, &current_token)).type == ERROR_NONE)
  {
    if (current_token.end - current_token.beginning == 0)
    {
      break;
    }

    // tokens -> the new one
    if (tokens)
    {
      // overwrite tokens->next
      token_it->next = token_create();
      memcpy(token_it->next, &current_token, sizeof(Token));
      token_it = token_it->next;
    }
    else
    {
      // overwrite tokens
      tokens = token_create();
      memcpy(tokens, &current_token, sizeof(Token));
      token_it = tokens;
    }
  };

  print_tokens(tokens);

  // Loop through tokens in attempt to create an AST
  Node *root = calloc(1, sizeof(Node));
  assert(root && "Failed to allocate memory for AST node");
  token_it = tokens;

  while (token_it)
  {
    // TODO: Map constructs from the language and attempt to create nodes (to the AST)

    if (token_string_equalp(":", token_it))
    {
      printf("Found `:` at token\n");
      // Look ahaed for next one
      if (token_it->next && token_string_equalp("=", token_it->next))
      {
        // Token *equals = token_it->next;
        printf("Found assignment\n");
      }
      else if (token_string_equalp("integer", token_it->next))
      {
        // TODO: Make helper to check if string is type name
        printf("Found (hopefully) a variable declaration\n");
      }
    }

    token_it = token_it->next;
  }

  free_tokens(tokens);
  return err;
}

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    print_usage(argv);
    exit(0);
  }

  char *path = argv[1];
  char *contents = file_contents(path);
  if (contents)
  {
    // printf("Contents of %s:\n---\n\"%s\"\n---\n", path, contents);

    Node expression;
    Error err = parse_expr(contents, &expression);
    print_error(err);

    free(contents);
  }

  return 0;
}