import java.util.List;
import java.util.ArrayList;

class LoxList {
    private final List<Object> elements;
    
    LoxList(List<Object> elements) {
        this.elements = elements;
    }
    
    int len() {
        return elements.size();
    }
    
    void append(Object element) {
        elements.add(element);
    }
    
    Object get(Token leftBracket, int index) {
        if (index >= len() || index < 0) {
            throw new RuntimeError(leftBracket, String.format("Index %d out of bounds.", index));
        }
        
        return elements.get(index);
    }
    
    Object assign(Token equal, int index, Object value) {
        if (index >= len() || index < 0) {
            throw new RuntimeError(equal, String.format("Index %d out of bounds.", index));
        }
        
        return elements.set(index, value);
    }
    
    void erase(Token token, int index) {
        if (index >= len() || index < 0) {
            throw new RuntimeError(token, String.format("Index %d out of bounds.", index));
        }
        
        elements.remove(index);
    }
    
    static LoxList merge(LoxList a, LoxList b) {
        List<Object> elements = new ArrayList<>();
        
        for (Object obj : a.elements) {
            elements.add(obj);
        }
        
        for (Object obj : b.elements) {
            elements.add(obj);
        }
        
        return new LoxList(elements);
    }
    
    @Override
    public String toString() {
        String s = "[";
        
        boolean first = true;
        for (Object obj : elements) {
           if (!first)  {
               s += ", ";
           }
           first = false;
           
           s += obj.toString();
        }
        
        s += "]";
        
        return s;
    }
}