# 📜 Complete Grammar Specification (EBNF-Style)

Based on your parser code, here is the **complete and corrected** formal grammar for your game scripting language.

---

## 🔑 Token Definitions (Terminals)

```ebnf
(* ===== KEYWORDS ===== *)
ROOM          = "room"
ACTION        = "action"
ITEM          = "item"
NPC           = "npc"
EXIT          = "exit"
IF            = "if"
ELSE          = "else"
PRINT         = "print"
REMOVE        = "remove"
PLAYER        = "player"
TRUE          = "true"
FALSE         = "false"
DESCRIPTION   = "description"
CURRENT_ROOM  = "current_room"

(* ===== LITERALS & IDENTIFIERS ===== *)
STRING        = '"' { <any char except '"'> | '\n' } '"'    (* double-quoted string *)
NUMBER        = [0-9]+                                       (* integer literal *)
IDENTIFIER    = [a-zA-Z_][a-zA-Z0-9_]*                       (* names: the_ring, bag_end, etc. *)

(* ===== CONTEXT-SENSITIVE TOKENS (emitted by lexer after '.') ===== *)
PLAYER_ATTR   = identifier immediately following "player."   (* e.g., inventory, win, has_item *)
ROOM_ATTR     = identifier immediately following "room." or another identifier

(* ===== OPERATORS & PUNCTUATION ===== *)
DOT           = "."
COLON         = ":"
COMMA         = ","
LEFT_BRACE    = "{"
RIGHT_BRACE   = "}"
LEFT_PAREN    = "("
RIGHT_PAREN   = ")"
EQUAL         = "="
EQUAL_EQUAL   = "=="
PLUS_EQUAL    = "+="
MINUS_EQUAL   = "-="
GREATER       = ">"
GREATER_EQUAL = ">="
LESS          = "<"
LESS_EQUAL    = "<="
AND_AND       = "&&"

(* ===== SPECIAL ===== *)
EOF           = <end of file>
ERROR         = <lexical error>
```

---

## 🏗️ Grammar Rules (Non-Terminals)

### 🌐 Top-Level Structure

```ebnf
program         → declaration* EOF

declaration     → room_decl | action_decl
```

### 🏠 Room Declaration

```ebnf
room_decl       → ROOM STRING LEFT_BRACE room_body* RIGHT_BRACE

room_body       → description_decl
                | item_decl
                | npc_decl
                | exit_decl

description_decl → DESCRIPTION STRING

item_decl       → ITEM IDENTIFIER LEFT_BRACE property_list? RIGHT_BRACE

npc_decl        → NPC IDENTIFIER LEFT_BRACE property_list? RIGHT_BRACE

exit_decl       → EXIT direction STRING           (* ← COMPLETE EXIT RULE *)

direction       → IDENTIFIER                       (* "north", "south", "east", "west", etc. *)

property_list   → property (COMMA property)*

property        → IDENTIFIER COLON literal
```

### ⚡ Action Declaration

```ebnf
action_decl     → ACTION STRING LEFT_BRACE statement* RIGHT_BRACE
```

### 📝 Statements (Inside Actions / If-Blocks)

```ebnf
statement       → print_stmt
                | remove_stmt
                | if_stmt
                | assignment_stmt

print_stmt      → PRINT STRING

remove_stmt     → REMOVE IDENTIFIER

if_stmt         → IF condition LEFT_BRACE statement* RIGHT_BRACE
                  (ELSE LEFT_BRACE statement* RIGHT_BRACE)?

assignment_stmt → PLAYER DOT attribute_name assign_op literal

attribute_name  → PLAYER_ATTR | IDENTIFIER

assign_op       → EQUAL | PLUS_EQUAL
```

### 🔍 Conditions (For If-Statements)

```ebnf
condition       → condition_primary (AND_AND condition_primary)*

condition_primary → player_has_item_condition
                  | player_attr_condition
                  | current_room_condition
                  | room_attr_condition

(* player.has_item(item_name) *)
player_has_item_condition
                → PLAYER DOT "has_item" LEFT_PAREN IDENTIFIER RIGHT_PAREN

(* player.attribute OP literal *)
player_attr_condition
                → PLAYER DOT PLAYER_ATTR comparison_op literal

(* current_room == "room_name" *)
current_room_condition
                → CURRENT_ROOM EQUAL_EQUAL STRING

(* room.attribute OP literal - future extension *)
room_attr_condition
                → ROOM DOT ROOM_ATTR comparison_op literal

comparison_op   → EQUAL_EQUAL | GREATER | GREATER_EQUAL | LESS | LESS_EQUAL
```

### 🔢 Literals (Values)

```ebnf
literal         → NUMBER
                | STRING
                | TRUE
                | FALSE
                | IDENTIFIER          (* for item names, room names, etc. *)
```

---

## 🧩 Visual Grammar Tree

```
program
├── declaration*
│   ├── room_decl
│   │   ├── "room"
│   │   ├── STRING (room name)
│   │   ├── "{"
│   │   ├── room_body*
│   │   │   ├── description_decl → "description" STRING
│   │   │   ├── item_decl → "item" ID { key: value, ... }
│   │   │   ├── npc_decl → "npc" ID { key: value, ... }
│   │   │   └── exit_decl → "exit" direction STRING  ← ✅ COMPLETE
│   │   │                    ├── "exit"
│   │   │                    ├── direction (IDENTIFIER: north/south/east/west)
│   │   │                    └── target room name (STRING)
│   │   └── "}"
│   │
│   └── action_decl
│       ├── "action"
│       ├── STRING (action name)
│       ├── "{"
│       ├── statement*
│       │   ├── print_stmt → "print" STRING
│       │   ├── remove_stmt → "remove" ID
│       │   ├── if_stmt → "if" condition { stmt* } [else { stmt* }]
│       │   └── assignment_stmt → player.attr [=|+=] literal
│       └── "}"
│
└── EOF
```

---

## 🎮 Complete Example: Valid Program

```text
room "bag_end" {
    description "A cozy hobbit hole."
    item the_ring { power: 100, type: "artifact" }
    npc gandalf { health: 100, friendly: true }
    exit east "buckland"
    exit west "hobbiton"
}

room "mount_doom" {
    description "The fiery Cracks of Doom."
    npc sauron { health: 999, hostile: true }
    exit west "mordor_outer"
}

action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "The Precious is yours."
    }
}

action "destroy ring" {
    if current_room == "mount_doom" && player.has_item(the_ring) {
        remove the_ring
        print "The ring is destroyed!"
        player.win = true
    } else {
        print "You cannot do that here."
    }
}

action "travel north" {
    if player.health >= 50 {
        print "You venture north."
    }
}
```

---

## ⚠️ Grammar Notes & Constraints

| Feature                         | Explanation                                                                                                                   |
| ------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| **Context-Sensitive Lexing**    | `PLAYER_ATTR`/`ROOM_ATTR` tokens are emitted only after `player.` or `room.` — lexer tracks `lastEmitted` and `lastBeforeDot` |
| **No Semicolons**               | Statements are separated by newlines/braces, not `;`                                                                          |
| **Properties Use Commas**       | `item { power: 100, type: "artifact" }` — comma-separated key:value pairs                                                     |
| **Exit Syntax**                 | `exit direction "target"` where `direction` is an IDENTIFIER (north/south/east/west) and target is a STRING                   |
| **Condition Restrictions**      | `current_room` only supports `==`; `player.has_item()` takes no operator                                                      |
| **Assignment Targets**          | Only `player.attribute` can be assigned (not arbitrary identifiers)                                                           |
| **Literal Types in Conditions** | RHS of comparisons can be `STRING`, `NUMBER`, `BOOL`, or `IDENTIFIER`                                                         |

---

## 🔄 Complete EBNF Summary (Compact)

```ebnf
program         = { declaration }, EOF ;
declaration     = room_decl | action_decl ;

room_decl       = "room", STRING, "{", { room_body }, "}" ;
room_body       = description_decl | item_decl | npc_decl | exit_decl ;
description_decl = "description", STRING ;
item_decl       = "item", IDENTIFIER, "{", [ property_list ], "}" ;
npc_decl        = "npc", IDENTIFIER, "{", [ property_list ], "}" ;
exit_decl       = "exit", direction, STRING ;              (* ✅ COMPLETE *)
direction       = IDENTIFIER ;                              (* north/south/east/west *)
property_list   = property, { ",", property } ;
property        = IDENTIFIER, ":", literal ;

action_decl     = "action", STRING, "{", { statement }, "}" ;

statement       = print_stmt | remove_stmt | if_stmt | assignment_stmt ;
print_stmt      = "print", STRING ;
remove_stmt     = "remove", IDENTIFIER ;
if_stmt         = "if", condition, "{", { statement }, "}",
                  [ "else", "{", { statement }, "}" ] ;
assignment_stmt = "player", ".", attribute_name, assign_op, literal ;
attribute_name  = PLAYER_ATTR | IDENTIFIER ;
assign_op       = "=" | "+=" ;

condition       = condition_primary, { "&&", condition_primary } ;
condition_primary = "player", ".", "has_item", "(", IDENTIFIER, ")"
                  | "player", ".", PLAYER_ATTR, comparison_op, literal
                  | "current_room", "==", STRING
                  | "room", ".", ROOM_ATTR, comparison_op, literal ;
comparison_op   = "==" | ">" | ">=" | "<" | "<=" ;

literal         = NUMBER | STRING | "true" | "false" | IDENTIFIER ;
```

_(Legend: `IDENTIFIER` = generic name token; `PLAYER_ATTR`/`ROOM_ATTR` = context-sensitive tokens)_

---

## 🧪 Grammar Validation Checklist

| Rule                      | Tested By                                                          |
| ------------------------- | ------------------------------------------------------------------ |
| `exit direction "target"` | `ParserTest.ParsesExitDeclaration` ✅                              |
| `player.attr += value`    | `ParserTest.ParsesPlayerPlusAssign` ✅                             |
| `player.attr = value`     | `ParserTest.ParsesPlayerAssignment` ✅                             |
| `if current_room == "x"`  | `ParserTest.ParsesCurrentRoomCondition` ✅                         |
| `player.has_item(x)`      | `ParserTest.RecognizesPlayerHasItemAsContextSensitiveAttribute` ✅ |
| Nested `if/else` blocks   | `ParserTest.ParsesFullWorldScript` ✅                              |
| Error recovery            | `ParserTest.FailsOnMissingRoomBrace` ✅                            |

---

## 🚀 Next Steps

With this **complete grammar**, you can now:

1. ✅ **Add syntax validation** for `direction` values (north/south/east/west) if desired
2. ✅ **Generate documentation** or a language reference for users
3. ✅ **Build tooling**: syntax highlighter, autocomplete, linter
4. ✅ **Extend safely**: add `while`, `function`, `inventory` commands by updating this spec first

### Optional Extensions (Future)

```ebnf
(* Add to statement *)
statement       → ... | while_stmt | function_call_stmt ;

while_stmt      → "while", condition, "{", { statement }, "}" ;

function_call_stmt → IDENTIFIER LEFT_PAREN [ literal ("," literal)* ] RIGHT_PAREN ;

(* Add to room_body *)
room_body       → ... | trigger_decl ;
trigger_decl    → "on" IDENTIFIER LEFT_BRACE statement* RIGHT_BRACE ;
```

---
