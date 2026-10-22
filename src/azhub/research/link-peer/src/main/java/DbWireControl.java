/* SPDX-License-Identifier: GPL-3.0-only */
import java.io.*;
import java.nio.channels.Channels;
import java.nio.file.*;
import java.util.Arrays;
import org.deepsymmetry.beatlink.dbserver.*;

/** Independent pinned Beat Link codec control; files contain synthetic data only. */
public final class DbWireControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=2) throw new IllegalArgumentException("generate|verify PATH");
        Message[] messages={
            new Message(1,0x4101,new NumberField(7),new StringField("Fixture \ud83c\udfb5"),new BinaryField(new byte[]{1,2,3})),
            new Message(2,0x3000,new NumberField(0),new BinaryField(new byte[0]),new NumberField(19)),
            new Message(3,0x4000,new NumberField(0xffffffffL),new StringField(""))
        };
        if(args[0].equals("generate")) {
            try(var output=Channels.newChannel(Files.newOutputStream(Path.of(args[1]),StandardOpenOption.CREATE_NEW))) {
                for(Message message:messages) message.write(output);
            }
        } else if(args[0].equals("verify")) {
            try(var input=new DataInputStream(Files.newInputStream(Path.of(args[1])))) {
                for(Message expected:messages) {
                    Message actual=Message.read(input);
                    if(actual.transaction.getValue()!=expected.transaction.getValue() ||
                       actual.messageType.getValue()!=expected.messageType.getValue() ||
                       actual.arguments.size()!=expected.arguments.size()) throw new AssertionError("Header mismatch");
                    for(int i=0;i<actual.arguments.size();++i) {
                        Field a=actual.arguments.get(i),b=expected.arguments.get(i);
                        boolean equal=a instanceof NumberField && b instanceof NumberField ?
                            ((NumberField)a).getValue()==((NumberField)b).getValue() :
                            a instanceof StringField && b instanceof StringField ?
                            ((StringField)a).getValue().equals(((StringField)b).getValue()) :
                            a instanceof BinaryField && b instanceof BinaryField &&
                            Arrays.equals(((BinaryField)a).getValueAsArray(),((BinaryField)b).getValueAsArray());
                        if(!equal) throw new AssertionError("Argument mismatch");
                    }
                }
                if(input.read()!=-1) throw new AssertionError("Trailing bytes");
            }
            System.out.println("PASS: unchanged Beat Link codec reads3 C-normalized Unicode/binary/empty messages exactly");
        } else throw new IllegalArgumentException("Unknown mode");
    }
}
