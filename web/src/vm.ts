export type Bytecode = {
  world: Room[];
  start_room: string;
  actions: Action[];
};

export type Room = {
  name: string;
  description: string;
  items: Entity[];
  npcs: Entity[];
  exits: Exit[];
};

export type Entity = {
  name: string;
  properties: Record<string, unknown>;
};

export type Exit = {
  direction: string;
  target: string;
};

type Action = {
  name: string;
  instructions: Instruction[];
};

type Instruction = {
  op: string;
  dest: string;
  src1: string;
  src2: string;
  int: number;
  bool: boolean;
  str: string;
};

export type GameSnapshot = {
  currentRoom: string;
  inventory: string[];
  won: boolean;
  hints: string[];
  log: string[];
};

export class DurinsVM {
  private state = {
    currentRoom: "",
    inventory: new Set<string>(),
    intAttrs: new Map<string, number>(),
    boolAttrs: new Map<string, boolean>(),
    strAttrs: new Map<string, string>()
  };

  private log: string[] = [];
  private ended = false;

  constructor(private readonly bytecode: Bytecode) {
    this.state.currentRoom = bytecode.start_room || bytecode.world[0]?.name || "";
    this.log.push("DURIN'S CODE");
    this.log.push("Read the room. Type any command shown below.");
    this.describeRoom();
  }

  snapshot(): GameSnapshot {
    return {
      currentRoom: this.state.currentRoom,
      inventory: [...this.state.inventory].sort(),
      won: this.state.boolAttrs.get("win") === true,
      hints: this.hints(),
      log: [...this.log]
    };
  }

  submit(input: string): GameSnapshot {
    const command = input.trim();
    if (!command) return this.snapshot();
    this.log.push(`> ${command}`);

    if (this.ended) {
      this.log.push("The quest has already ended.");
      return this.snapshot();
    }

    if (command === "help" || command === "?") {
      this.log.push("look | go <direction> | inventory | quit | any action name");
      return this.snapshot();
    }

    if (command === "quit" || command === "exit") {
      this.log.push("Farewell, adventurer.");
      this.ended = true;
      return this.snapshot();
    }

    if (command === "look") {
      this.describeRoom();
      return this.snapshot();
    }

    if (command === "inventory" || command === "inv") {
      if (this.state.inventory.size === 0) {
        this.log.push("bag: empty");
      } else {
        this.log.push(`bag: ${[...this.state.inventory].sort().join(", ")}`);
      }
      return this.snapshot();
    }

    if (command.startsWith("go ")) {
      this.go(command.slice(3).trim());
      return this.snapshot();
    }

    if (!this.executeAction(command)) {
      this.log.push(`"${command}" is not a valid command here.`);
      this.log.push(`Try: ${this.hints().join(" | ")}`);
    }

    if (this.state.boolAttrs.get("win") === true) {
      this.log.push("YOU WIN! The quest is complete.");
      this.ended = true;
    }

    return this.snapshot();
  }

  private room(): Room | undefined {
    return this.bytecode.world.find((room) => room.name === this.state.currentRoom);
  }

  private pretty(id: string): string {
    return id
      .split("_")
      .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
      .join(" ");
  }

  private describeRoom(): void {
    const room = this.room();
    if (!room) {
      this.log.push(`Unknown room: ${this.pretty(this.state.currentRoom)}`);
      return;
    }

    this.log.push("");
    this.log.push(this.pretty(room.name));
    this.log.push(room.description);

    if (room.items.length > 0) {
      this.log.push(`You see: ${room.items.map((item) => this.pretty(item.name)).join(", ")}`);
    }

    if (room.npcs.length > 0) {
      this.log.push(`Beware: ${room.npcs.map((npc) => this.pretty(npc.name)).join(", ")}`);
    }

    for (const exit of room.exits) {
      this.log.push(`go ${exit.direction} -> ${this.pretty(exit.target)}`);
    }
  }

  private go(direction: string): void {
    const room = this.room();
    const exit = room?.exits.find((candidate) => candidate.direction === direction);
    if (!room || !exit) {
      const exits = room?.exits.map((candidate) => candidate.direction).join(", ") || "none";
      this.log.push(`Can't go "${direction}" from here. You can go: ${exits}`);
      return;
    }
    this.state.currentRoom = exit.target;
    this.describeRoom();
  }

  private hints(): string[] {
    const commands: string[] = [];
    const room = this.room();
    if (room) {
      commands.push(...room.exits.map((exit) => `go ${exit.direction}`));
    }

    for (const action of this.bytecode.actions) {
      let roomOk = true;
      let hasRoomCheck = false;
      let hasItemCheck = false;
      let itemPresent = true;

      for (const instr of action.instructions) {
        if (instr.op === "CHECK_ROOM") {
          hasRoomCheck = true;
          roomOk = instr.str === this.state.currentRoom;
          break;
        }
        if (instr.op === "HAS_ITEM" && !hasItemCheck) {
          hasItemCheck = true;
          itemPresent = this.state.inventory.has(instr.str);
        }
      }

      if (!hasRoomCheck && hasItemCheck && !itemPresent) continue;
      if (roomOk) commands.push(action.name);
    }

    commands.push("look", "inventory");
    return commands;
  }

  private executeAction(actionName: string): boolean {
    const action = this.bytecode.actions.find((candidate) => candidate.name === actionName);
    if (!action) return false;

    const labelMap = new Map<string, number>();
    action.instructions.forEach((instr, index) => {
      if (instr.op === "LABEL") labelMap.set(instr.dest, index);
    });

    const temps = new Map<string, boolean>();
    let ip = 0;

    while (ip < action.instructions.length) {
      const instr = action.instructions[ip];

      switch (instr.op) {
        case "PRINT":
          this.log.push(instr.str);
          break;
        case "REMOVE_ITEM":
          this.state.inventory.delete(instr.str);
          break;
        case "SET_PLAYER_ATTR":
          this.setPlayerAttr(instr);
          break;
        case "CHECK_ROOM":
          temps.set(instr.dest, this.state.currentRoom === instr.str);
          break;
        case "HAS_ITEM":
          temps.set(instr.dest, this.state.inventory.has(instr.str));
          break;
        case "COMPARE_EQ":
          temps.set(instr.dest, this.compareEq(instr));
          break;
        case "COMPARE_GT":
          temps.set(instr.dest, (this.state.intAttrs.get(instr.src1) ?? Number.NaN) > instr.int);
          break;
        case "COMPARE_GTE":
          temps.set(instr.dest, (this.state.intAttrs.get(instr.src1) ?? Number.NaN) >= instr.int);
          break;
        case "COMPARE_LT":
          temps.set(instr.dest, (this.state.intAttrs.get(instr.src1) ?? Number.NaN) < instr.int);
          break;
        case "COMPARE_LTE":
          temps.set(instr.dest, (this.state.intAttrs.get(instr.src1) ?? Number.NaN) <= instr.int);
          break;
        case "AND":
          temps.set(instr.dest, Boolean(temps.get(instr.src1)) && Boolean(temps.get(instr.src2)));
          break;
        case "OR":
          temps.set(instr.dest, Boolean(temps.get(instr.src1)) || Boolean(temps.get(instr.src2)));
          break;
        case "JUMP_IF_FALSE":
          if (!Boolean(temps.get(instr.src1)) && labelMap.has(instr.dest)) {
            ip = labelMap.get(instr.dest)!;
            continue;
          }
          break;
        case "JUMP":
          if (labelMap.has(instr.dest)) {
            ip = labelMap.get(instr.dest)!;
            continue;
          }
          break;
      }

      ip += 1;
    }

    return true;
  }

  private setPlayerAttr(instr: Instruction): void {
    if (instr.src1 === "=") {
      if (instr.str) this.state.strAttrs.set(instr.dest, instr.str);
      else if (instr.int !== 0) this.state.intAttrs.set(instr.dest, instr.int);
      else this.state.boolAttrs.set(instr.dest, instr.bool);
      return;
    }

    if (instr.src1 === "+=") {
      if (instr.str) this.state.inventory.add(instr.str);
      else this.state.intAttrs.set(instr.dest, (this.state.intAttrs.get(instr.dest) || 0) + instr.int);
      return;
    }

    if (instr.src1 === "-=") {
      if (instr.str) this.state.inventory.delete(instr.str);
      else this.state.intAttrs.set(instr.dest, (this.state.intAttrs.get(instr.dest) || 0) - instr.int);
    }
  }

  private compareEq(instr: Instruction): boolean {
    if (this.state.intAttrs.has(instr.src1)) {
      return this.state.intAttrs.get(instr.src1) === instr.int;
    }
    if (this.state.boolAttrs.has(instr.src1)) {
      return this.state.boolAttrs.get(instr.src1) === instr.bool;
    }
    if (this.state.strAttrs.has(instr.src1)) {
      return this.state.strAttrs.get(instr.src1) === instr.str;
    }
    return false;
  }
}

export function parseBytecode(jsonText: string): Bytecode {
  const parsed = JSON.parse(jsonText) as Bytecode;
  if (!Array.isArray(parsed.world) || !Array.isArray(parsed.actions)) {
    throw new Error("Bytecode is missing world/actions arrays.");
  }
  return parsed;
}
