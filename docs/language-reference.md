# Durin's Code — Language Reference Manual

**CS4031 Compiler Construction · Spring 2026**

---

## 1. Introduction

Durin's Code (file extension `.dc`) is a declarative domain-specific language for authoring interactive text-adventure games. A program defines a game *world* (rooms, items, NPCs) and a set of *actions* (event handlers triggered by player input). The compiler validates the world graph statically, generates optimised TAC bytecode, and executes it inside a terminal VM.

---

## 2. Lexical Structure

### 2.1 Character Set
UTF-8 source files. Line endings: `\n` (Unix) or `\r\n` (Windows) — both treated identically.

### 2.2 Comments
```
// This is a line comment. Everything after // until end-of-line is ignored.
```
Block comments are not supported.

### 2.3 Whitespace
Spaces, tabs, and newlines are insignificant except as token separators.

### 2.4 Identifiers
```
identifier  ::=  [_a-zA-Z] [_a-zA-Z0-9]*
```
Identifiers are case-sensitive. Leading underscores are permitted.

### 2.5 Keywords
The following words are reserved and cannot be used as identifiers:

| Keyword | Token |
|---|---|
| `room` | `TOKEN_ROOM` |
| `action` | `TOKEN_ACTION` |
| `item` | `TOKEN_ITEM` |
| `npc` | `TOKEN_NPC` |
| `exit` | `TOKEN_EXIT` |
| `if` | `TOKEN_IF` |
| `else` | `TOKEN_ELSE` |
| `print` | `TOKEN_PRINT` |
| `remove` | `TOKEN_REMOVE` |
| `player` | `TOKEN_PLAYER` |
| `true` | `TOKEN_TRUE` |
| `false` | `TOKEN_FALSE` |
| `description` | `TOKEN_DESCRIPTION` |
| `current_room` | `TOKEN_CURRENT_ROOM` |

### 2.6 Literals

**String literal** — enclosed in double quotes, single line:
```
"hello world"
"The cozy hole of a Hobbit."
```

**Integer literal** — sequence of decimal digits:
```
100    999    0
```
Floating-point numbers are not supported (compile error).

**Boolean literals** — `true`, `false`.

### 2.7 Operators and Punctuation

| Symbol | Token |
|---|---|
| `{` | `TOKEN_LEFT_BRACE` |
| `}` | `TOKEN_RIGHT_BRACE` |
| `(` | `TOKEN_LEFT_PAREN` |
| `)` | `TOKEN_RIGHT_PAREN` |
| `,` | `TOKEN_COMMA` |
| `:` | `TOKEN_COLON` |
| `;` | `TOKEN_SEMICOLON` |
| `.` | `TOKEN_DOT` |
| `=` | `TOKEN_EQUAL` |
| `==` | `TOKEN_EQUAL_EQUAL` |
| `+=` | `TOKEN_PLUS_EQUAL` |
| `-=` | `TOKEN_MINUS_EQUAL` |
| `+` | `TOKEN_PLUS` |
| `-` | `TOKEN_MINUS` |
| `>` | `TOKEN_GREATER` |
| `>=` | `TOKEN_GREATER_EQUAL` |
| `<` | `TOKEN_LESS` |
| `<=` | `TOKEN_LESS_EQUAL` |
| `&&` | `TOKEN_AND_AND` |
| `\|\|` | `TOKEN_OR_OR` |

### 2.8 Context-Sensitive Dot Tokens
After a `.` the lexer inspects the token immediately before the dot:
- `player.` → next identifier becomes `TOKEN_PLAYER_ATTR`
- `room_id.` or `identifier.` → next identifier becomes `TOKEN_ROOM_ATTR`

---

## 3. Grammar (EBNF)

```ebnf
program         ::=  declaration* EOF

declaration     ::=  room_decl
                   | action_decl

(* ── World ─────────────────────────────────── *)

room_decl       ::=  "room" STRING "{" room_body* "}"

room_body       ::=  description_stmt
                   | item_decl
                   | npc_decl
                   | exit_decl

description_stmt ::= "description" STRING

item_decl       ::=  "item" IDENTIFIER "{" prop_list "}"

npc_decl        ::=  "npc"  IDENTIFIER "{" prop_list "}"

prop_list       ::=  prop ("," prop)*
prop            ::=  IDENTIFIER ":" (NUMBER | STRING | BOOLEAN)

exit_decl       ::=  "exit" IDENTIFIER STRING
                 (*         ^direction  ^target_room_id *)

(* ── Actions ────────────────────────────────── *)

action_decl     ::=  "action" STRING "{" statement* "}"

statement       ::=  print_stmt
                   | remove_stmt
                   | assign_stmt
                   | if_stmt

print_stmt      ::=  "print" STRING

remove_stmt     ::=  "remove" IDENTIFIER

assign_stmt     ::=  player_attr_ref ("=" | "+=" | "-=") (IDENTIFIER | NUMBER | STRING | BOOLEAN)

player_attr_ref ::=  "player" "." IDENTIFIER

if_stmt         ::=  "if" condition "{" statement* "}" ( "else" "{" statement* "}" )?

(* ── Conditions ─────────────────────────────── *)

condition       ::=  simple_cond ( ("&&" | "||") simple_cond )*

simple_cond     ::=  room_check
                   | has_item_check
                   | comparison

room_check      ::=  "current_room" "==" STRING

has_item_check  ::=  "player" "." "has_item" "(" IDENTIFIER ")"

comparison      ::=  player_attr_ref ("==" | ">" | ">=" | "<" | "<=") (NUMBER | STRING | BOOLEAN)

(* ── Terminals ───────────────────────────────── *)

STRING          ::=  '"' [^"\n]* '"'
NUMBER          ::=  [0-9]+
BOOLEAN         ::=  "true" | "false"
IDENTIFIER      ::=  [_a-zA-Z] [_a-zA-Z0-9]*
```

---

## 4. Declarations

### 4.1 Room Declaration
Defines a location in the world graph.

```dc
room "bag_end" {
    description "The cozy hole of a Hobbit."
    item the_ring { power: 100, type: "artifact" }
    npc gandalf   { health: 500, hostile: false }
    exit east "buckland"
    exit west "shire"
}
```

- `description` — sets the text displayed by the `look` command (exactly one per room).
- `item` — places a named item in the room with a property map.
- `npc` — places a named NPC in the room with a property map.
- `exit` — declares a directed edge to another room; direction is a free identifier (`north`, `south`, `east`, `west`, or any word).

**Semantic constraints:**
- Room IDs must be unique across the program.
- Exit target strings must reference a declared room ID.
- Duplicate exit directions in the same room are a semantic error.

### 4.2 Action Declaration
Defines a named event handler triggered by player input.

```dc
action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "The Precious is yours."
    }
}
```

Action names are strings (may contain spaces). At runtime the player types the action name verbatim.

---

## 5. Statements

### 5.1 `print`
Displays a string to the terminal.
```dc
print "You cannot do that here."
```

### 5.2 `remove`
Removes a named item from the game world (and from the player's inventory if present).
```dc
remove the_ring
```
**Semantic constraint:** the identifier must name a declared item.

### 5.3 Assignment
Sets or modifies a player attribute.
```dc
player.win       = true          // boolean assignment
player.health    = 100           // integer assignment
player.inventory += the_ring     // add item to inventory
player.inventory -= the_ring     // remove item from inventory
```

### 5.4 `if` / `else`
Conditional execution. `else` branch is optional.
```dc
if current_room == "mount_doom" && player.has_item(the_ring) {
    remove the_ring
    player.win = true
} else {
    print "You cannot do that here."
}
```

---

## 6. Conditions

| Form | Meaning |
|---|---|
| `current_room == "id"` | Player is currently in room `id` |
| `player.has_item(name)` | Player's inventory contains `name` |
| `player.attr == value` | Player attribute comparison |
| `player.attr > value` | Greater-than comparison |
| `cond1 && cond2` | Logical AND (both must be true) |
| `cond1 \|\| cond2` | Logical OR (either must be true) |

---

## 7. Type System

Durin's Code is statically typed at the property level. Property values in `item`/`npc` blocks must be one of:

| Type | Example |
|---|---|
| Integer | `100` |
| String | `"artifact"` |
| Boolean | `true` / `false` |

Player attributes are dynamically typed (assigned value determines type). Floating-point numbers are rejected at lex time.

---

## 8. Scope Rules

- Room, item, NPC, and action names are all in one global flat namespace.
- Duplicate names within the same category are semantic errors.
- Item names declared inside rooms are also globally registered and may be referenced in any action.
- `player` is a built-in identifier; `player.X` accesses the player's runtime attribute `X`.

---

## 9. Error Reporting

### Lexer errors
| Error | Cause |
|---|---|
| `Unexpected character` | Character not in the language alphabet (e.g. `@`) |
| `Unterminated string.` | `"` opened but not closed before end-of-line |
| `Floating-point numbers are not supported.` | Digit sequence containing `.` |
| `Expected '&&'` | Single `&` not followed by `&` |
| `Expected '\|\|'` | Single `\|` not followed by `\|` |

### Semantic errors
| Error | Cause |
|---|---|
| `Duplicate room` | Two rooms with the same name |
| `Duplicate item` | Two items with the same name |
| `Duplicate action` | Two actions with the same name |
| `Exit target '…' not declared` | Exit points to non-existent room |
| `Duplicate exit direction` | Same direction used twice in one room |
| `remove '…' — unknown item` | remove references undeclared item |

---

## 10. Complete Example Program

```dc
// Middle-earth adventure

room "bag_end" {
    description "The cozy hole of a Hobbit. A golden ring glints on the table."
    item the_ring { power: 100, type: "artifact" }
    exit east "buckland"
}

room "buckland" {
    description "A quiet village by the river."
    exit west "bag_end"
    exit east "mount_doom"
}

room "mount_doom" {
    description "The air is thick with ash. The fiery Cracks of Doom loom ahead."
    npc sauron { health: 999, hostile: true }
    exit west "buckland"
}

action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "The Precious is yours."
    } else {
        print "There is nothing to take here."
    }
}

action "destroy ring" {
    if current_room == "mount_doom" && player.has_item(the_ring) {
        remove the_ring
        print "The ring is consumed by fire! Middle-earth is saved."
        player.win = true
    } else {
        print "You cannot do that here."
    }
}
```
