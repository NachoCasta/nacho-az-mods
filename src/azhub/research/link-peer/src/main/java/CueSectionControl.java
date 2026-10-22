/* SPDX-License-Identifier: GPL-3.0-only */
import org.deepsymmetry.cratedigger.pdb.RekordboxAnlz;
/** Independent numeric cue oracle; private analysis paths stay local. */
public final class CueSectionControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException("ANALYSIS_FILE");
        for(var section:RekordboxAnlz.fromFile(args[0]).sections()) {
            if(section.body() instanceof RekordboxAnlz.CueTag) {
                var tag=(RekordboxAnlz.CueTag)section.body();
                for(var cue:tag.cues()) System.out.println(tag.type().id()+"\t"+cue.hotCue()+"\t"+cue.type().id()+"\t"+cue.status().id()+"\t"+cue.time()+"\t"+cue.loopTime());
            }
        }
    }
}
