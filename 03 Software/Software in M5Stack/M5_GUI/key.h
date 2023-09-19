#pragma once

// Key class (name Button already defined for another class)

class Key {
  public:
    String label;
    void (*key_callback)();
    //int key_callback;

    Key();
    Key(String label, void (*key_callback)() );
};

Key::Key() { 
  this->label = ""; 
  this->key_callback = NULL; 
}

Key::Key(String label, void (*key_callback)() ) { 
//Key::Key(String label, int key_callback ) { 
  this->label = label; 
  this->key_callback = key_callback; 
}

