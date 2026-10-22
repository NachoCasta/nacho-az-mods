/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.file.*;
import java.security.MessageDigest;
import org.deepsymmetry.cratedigger.FileFetcher;

/** Explicit physical link-local transfer using the unmodified Crate Digger client. */
public final class PhysicalFileServiceClient {
 public static void main(String[] args) throws Exception {
  if(args.length!=5) throw new IllegalArgumentException("LOCAL_IPV4 SOURCE_IPV4 RELATIVE_PATH BYTES SHA256");
  InetAddress local=InetAddress.getByName(args[0]),target=InetAddress.getByName(args[1]);
  NetworkInterface nic=NetworkInterface.getByInetAddress(local);
  long size=Long.parseLong(args[3]);
  if(!(local instanceof Inet4Address) || !(target instanceof Inet4Address) || !local.isLinkLocalAddress() || !target.isLinkLocalAddress() || local.equals(target) || nic==null || !nic.isUp() || nic.isLoopback() || size<1 || size>32*1024*1024 || !args[4].matches("[0-9a-f]{64}"))
   throw new IllegalArgumentException("Assigned link-local Ethernet and bounded exact file reference required");
  Path relative=Path.of(args[2]);
  if(relative.isAbsolute() || relative.normalize().startsWith("..")) throw new IllegalArgumentException("Relative exported file required");
  Path output=Files.createTempFile(Path.of("local/diagnostics/physical-ethernet-library-v1"),"fetch-",".bin");
  try {
   var client=FileFetcher.getInstance();client.setRetransmitTimeout(500);
   client.fetch(target,"/C/",args[2],output.toFile());
   if(Files.size(output)!=size) throw new AssertionError("Exact file size mismatch");
   String hash=java.util.HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(Files.readAllBytes(output)));
   if(!hash.equals(args[4])) throw new AssertionError("Exact file hash mismatch");
   System.out.println("PASS: unchanged Crate Digger physical file transfer bytes="+size+" sha256="+hash+" source="+target.getHostAddress());
  } finally {Files.deleteIfExists(output);}
 }
}
