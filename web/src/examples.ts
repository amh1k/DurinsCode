export type Example = {
  name: string;
  source: string;
};

export const examples: Example[] = [
  {
    name: "01_hello_world.dc",
    source: `// Example 1: Hello World
// Demonstrates the minimal valid Durin's Code program.
// One room, one action, one print statement.

room "start" {
    description "You are standing in a white void. A single button sits before you."
    exit north "end"
}

room "end" {
    description "You have reached the end of the world."
    exit south "start"
}

action "press button" {
    if current_room == "start" {
        print "Hello, adventurer! Your journey begins."
    } else {
        print "There is no button here."
    }
}
`
  },
  {
    name: "02_inventory.dc",
    source: `// Example 2: Inventory System
// Demonstrates player.inventory += / -= and player.has_item() condition.

room "armoury" {
    description "Racks of weapons line the walls. A gleaming sword catches the light."
    item sword { damage: 50, type: "weapon" }
    exit east "throne_room"
}

room "throne_room" {
    description "The king sits on a golden throne. Guards eye you suspiciously."
    npc king { health: 100, hostile: false }
    exit west "armoury"
}

action "take sword" {
    if current_room == "armoury" {
        player.inventory += sword
        print "You take the sword. It feels well-balanced."
    } else {
        print "There is no sword here."
    }
}

action "drop sword" {
    if player.has_item(sword) {
        player.inventory -= sword
        print "You set the sword down on the ground."
    } else {
        print "You are not carrying a sword."
    }
}

action "threaten king" {
    if current_room == "throne_room" && player.has_item(sword) {
        print "You brandish the sword. The king raises an eyebrow."
        player.reputation = 0
    } else {
        print "You wave your empty hand. The king is unimpressed."
    }
}
`
  },
  {
    name: "03_multiroom.dc",
    source: `// Example 3: Multi-Room Navigation
// Demonstrates a connected world graph with four rooms and multiple exits.

room "village" {
    description "A peaceful village with cobblestone streets."
    item map { detail: "worn", pages: 10 }
    exit north "forest"
    exit east "market"
}

room "forest" {
    description "Tall pines block the sunlight. Something rustles in the undergrowth."
    npc wolf { health: 30, hostile: true }
    exit south "village"
    exit east "cave"
}

room "market" {
    description "Merchants hawk their wares. The smell of fresh bread fills the air."
    item bread { nutrition: 20, type: "food" }
    item potion { healing: 50, type: "consumable" }
    exit west "village"
}

room "cave" {
    description "A dark cave. Dripping water echoes. Treasure glints in the shadows."
    item treasure { value: 500, type: "gold" }
    exit west "forest"
}

action "take map" {
    if current_room == "village" {
        player.inventory += map
        print "You pick up the worn map. Useful for navigation."
    } else {
        print "There is no map here."
    }
}

action "take treasure" {
    if current_room == "cave" {
        player.inventory += treasure
        player.gold = 500
        print "You scoop up the gold coins. You are rich!"
        player.win = true
    } else {
        print "There is no treasure here."
    }
}

action "buy bread" {
    if current_room == "market" {
        player.inventory += bread
        print "The merchant smiles as you hand over a coin."
    } else {
        print "You are not at the market."
    }
}
`
  },
  {
    name: "04_middle_earth.dc",
    source: `// Example 4: Middle-Earth Adventure (Full Demo)
// Demonstrates rooms, items, NPCs, exits, conditions, inventory, remove,
// player attributes, and the win condition.

room "bag_end" {
    description "The cozy hole of a Hobbit. A golden ring glints on the table."
    item the_ring { power: 100, type: "artifact" }
    exit east "buckland"
}

room "buckland" {
    description "A quiet riverside village. The old ferry creaks at the dock."
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
        print "There is nothing of value here."
    }
}

action "examine ring" {
    if player.has_item(the_ring) {
        print "A plain golden band. It whispers your name."
    } else {
        print "You are not carrying any ring."
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
`
  },
  {
    name: "05_error_demo.dc",
    source: `// Example 5: Semantic Error Demo (intentionally broken)

room "dungeon" {
    description "A damp cell."
    exit north "tower"
}

room "dungeon" {
    description "A second dungeon with the same name."
    exit north "tower"
}

room "tower" {
    description "A tall stone tower."
    exit up "sky"
    exit down "dungeon"
}

room "courtyard" {
    description "An open courtyard."
    exit north "tower"
    exit north "dungeon"
}

action "grab ghost item" {
    remove phantom_item
}

action "check ghost item" {
    if player.has_item(phantom_item) {
        print "This should never compile."
    }
}
`
  }
];
