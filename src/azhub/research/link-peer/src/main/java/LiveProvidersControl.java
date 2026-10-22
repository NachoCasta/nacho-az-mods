/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import org.deepsymmetry.beatlink.*;

/** One unchanged DeviceFinder owns discovery; independent Clients browse all
 * source endpoints concurrently without competing for discovery port50000. */
public final class LiveProvidersControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=3) throw new IllegalArgumentException("PDB ORACLE_TSV SOURCE_COUNT");
        int[] targets;
        if(args[2].startsWith("--indices=")) targets=Arrays.stream(args[2].substring(10).split(",")).mapToInt(Integer::parseInt).toArray();
        else targets=java.util.stream.IntStream.range(0,Integer.parseInt(args[2])).toArray();
        int count=targets.length;
        Set<Integer> selected=new HashSet<>();for(int index:targets) if(index<0 || index>=8 || !selected.add(index)) throw new IllegalArgumentException("Distinct indices0..7");
        if(count<2 || count>8) throw new IllegalArgumentException("Count2..8");
        LiveProviderControl.validateNetwork();
        DeviceFinder finder=DeviceFinder.getInstance();
        ExecutorService pool=Executors.newFixedThreadPool(count);
        try {
            finder.start(); long deadline=System.nanoTime()+15_000_000_000L;
            Set<Integer> found=new HashSet<>();
            while(found.size()!=count && System.nanoTime()<deadline) {
                for(var device:finder.getCurrentDevices()) {
                    int index=device.getDeviceNumber()-41;
                    if(selected.contains(index) && device.getAddress().getHostAddress().equals(LiveProviderControl.sourceAddress(index))) found.add(index);
                }
                if(found.size()!=count) Thread.sleep(25);
            }
            if(found.size()!=count) throw new AssertionError("Missing independently discovered sources");
            List<Future<?>> jobs=new ArrayList<>();
            for(int index:targets) {
                int port;
                try(Socket query=new Socket(LiveProviderControl.sourceAddress(index),12523)) {
                    query.setSoTimeout(3000);
                    query.getOutputStream().write(new byte[]{0,0,0,15,'R','e','m','o','t','e','D','B','S','e','r','v','e','r',0});
                    byte[] reply=query.getInputStream().readNBytes(2);
                    if(reply.length!=2) throw new AssertionError("Port lookup");
                    port=((reply[0]&255)<<8)|(reply[1]&255);
                }
                System.out.println("SOURCE: index="+index+" DB port="+port);System.out.flush();
                final int source=index,metadata=port;
                jobs.add(pool.submit(()->{
                    try {
                        DbLibraryControl.main(new String[]{Integer.toString(metadata),args[0],args[1],"--emulator-link-"+(source+1)});
                        System.out.println("PASS: independently discovered and fully browsed source"+(source+1));
                    } catch(Exception failure) { throw new CompletionException(failure); }
                }));
            }
            for(Future<?> job:jobs) job.get(180,TimeUnit.SECONDS);
            System.out.println("PASS: unchanged DeviceFinder and concurrent complete Beat Link menus for sources="+count);
        } finally { pool.shutdownNow(); finder.stop(); }
    }
}
