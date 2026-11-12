# rhythm_gate

A rhythm gate implementation for the NTS-3 and Korg Logue SDK.  What is a rhythm gate?
It's a gate that opens and closes based on the rhythm of the music.  You can choose an even
rhythm based on the tempo (e.g., 1/2, 1/4, 1/8, or 1/16 notes), control it through MIDI CC
messages that define the rhythm to follow, or manually by tapping on the XY pad.

## Install

Download the latest release from the [releases page](https://github.com/chubbard/rhythm_gate/releases).

1. Download the nts3unit file
2. Connect your NTS-3 Kaoss Pad to your computer.
3. Open up Kontrol Editor.
4. Select the Generic FX tab in the UI.
5. Drag and drop the nts3unit file into the Generic FX tab to a free slot.
6. Click the Send button to send the rhythm_gate fx to the NTS-3.
7. Create a new fx preset that uses the rhythm_gate fx.

## Parameters

| Parameter | Description                                                                 | Values                       | Default Assign |
|-----------|-----------------------------------------------------------------------------|------------------------------|----------------|
| Attack    | Choose the attack time                                                      | 0.001 - 1.0                  | Pad X          |
| Release   | Choose the release time                                                     | 0.001 - 1.0                  | Pad Y          |
| Reduce    | When the gate closes reduce the incoming signal by this many dB             | -114.0 - 0.0                 | Depth          |
| Gate Type | Choose the rhythm division                                                  | 1/2, 1/4, 1/8, 1/16, CC, Pad |                |
| CC GATE   | Assign CC GATE to Pad X/Y or Depth on the device and send that control's CC | CC 12/13/14                  |                |

### Gate Type

- 1/2: Opens on every other beat
- 1/4: Opens on every quarter note
- 1/8: Opens on every eighth note
- 1/16: Opens on every sixteenth note
- CC: Opens/Close based on CC message(s)
- Pad: Opens on every pad press

### CC GATE

CC Gate is used to respond to MIDI CC messages sent to the NTS-3's MIDI channel. Assign the 
CC Gate parameter to one of Pad X (CC 12), Pad Y (CC 13), or Depth (CC 14). Send the corresponding
MIDI CC with a value greater than or equal to 50% to open the gate 
(e.g., MIDI CC value >= 64 opens the gate).
