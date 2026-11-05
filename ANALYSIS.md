<div align="center">

# The analysis

**The lameness measures, a first skeleton after the published methods.**

↑ [NIC-Strelka](README.md)

</div>

---

What the record is turned into for the vet: the asymmetry of the trunk's vertical movement,
stride by stride. The skeleton in [`analysis/lameness.py`](analysis/lameness.py) follows the
published methods below and passes its checks on synthetic trots
([`analysis/test_lameness.py`](analysis/test_lameness.py)); nothing in it is validated on an
animal. It is where the work starts, not where it ends.

## The published methods it follows

- **Head and pelvis, up and down.** Lameness at the trot is read from the vertical movement of
  the head (poll) and the pelvis (tuber sacrale): the difference between the two lowest points of
  a stride, reached in the two stances — MinDiff — and between the two highest, reached in the two
  flights — MaxDiff
  ([Animals, 2023](https://www.mdpi.com/2076-2615/13/3/390/htm)). The head stands for the
  forelimbs, the pelvis for the hindlimbs, the withers sometimes for a compensating lameness
  ([Sensors, 2023](https://www.mdpi.com/1424-8220/23/20/8414)).
- **One IMU, against an established system.** A single low-cost IMU on the sacrum agreed with an
  established IMU system to ±6,6 mm for MinDiff and ±4,3 mm for MaxDiff; its symmetry index was
  precise enough for mildly lame horses, MinDiff and MaxDiff less so
  ([Brighton, Olsen and Pfau, RVC](https://rvc-repository.worktribe.com/output/1402235)).
- **The first harmonic over the second.** A sound trot moves the trunk twice a stride, the second
  harmonic; an asymmetry adds the first. Their ratio measures the asymmetry
  ([Keegan's group, SciTePress 2012](https://www.scitepress.org/Papers/2012/38792/38792.pdf));
  the first harmonic's amplitude and phase describe a stride's asymmetry
  ([SLU](https://pub.epsilon.slu.se/id/document/20394709)).
- **Equinosis's thresholds and signs.** Its system calls a horse lame at ±6 mm of the head's Diff
  Max or Diff Min — 8,5 mm as their vector sum — and ±3 mm of the pelvis, over 25 strides or more
  with the deviation under the mean; the thresholds come from the 95 % intervals of repeated trials
  on 236 horses and are tied to its own processing, so another system derives its own. A positive
  difference names the right limb; Diff Min is the impact, Diff Max the push-off, the pelvis's two
  never summed ([Keegan et al., 2011](https://doi.org/10.2460/ajvr.72.9.1156); [Rhodin et al.,
  2016](https://pmc.ncbi.nlm.nih.gov/articles/PMC5032979/); [AAEP
  2022](https://pubs.aaep.org/0A4370h/68thAnnCon2022/html/359.html); [Macaire et al.,
  2022](https://pmc.ncbi.nlm.nih.gov/articles/PMC9774792/)).
- **A cow, the whole body.** Nine IMUs on cows with an induced forelimb lameness: 31 measures, most
  of them symmetries; the head and the neck changed most, in their first and second harmonics
  ([Leclercq et al., *Animal* 2025](https://pub.epsilon.slu.se/id/document/20449105)).

## What the skeleton does

1. The vertical acceleration, gravity removed, integrated twice in the frequency domain; what
   lies below half the stride frequency — the drift — is dropped.
2. The strides cut at their starts, taken from a limb unit.
3. Per stride: MinDiff, MaxDiff, and the ratio of the first harmonic to the second; the median
   over the strides.

## What it does not do yet

- **The vertical in the world.** The sensor turns with the body; its acceleration must first be
  turned into the world's frame and gravity taken off — an orientation filter from the gyro and
  the accelerometer.
- **The stride's start** from the limb unit: the published way takes the stance where the angular
  rate's variance falls and the foot's contact at the acceleration's peak, a pastern unit closer
  than a cannon's ([Tijssen et al., 2020](https://pmc.ncbi.nlm.nih.gov/articles/PMC7269263); [Briggs
  and Mazzà, 2021](https://pmc.ncbi.nlm.nih.gov/articles/PMC8312981/)).
- **Which limb.** The sign of MinDiff and MaxDiff, read against the reference limb's phase, names
  the side; the patterns that tell the limb are the vet's and the literature's.
- **The thresholds.** What value means lame, for a dog, a horse, a cow, comes from the studies and
  from Strelka's own measurements beside a system that exists.

## A patent to mind

The University of Missouri's US 7 601 126 B2, *Lameness evaluation systems and methods* (Keegan and
Pai), filed on 20 July 2007, covers sensors on the head, the pelvis and a limb whose vertical
patterns are compared with lameness patterns; its maintenance fees are paid and it stands to its
expiry, 20 July 2027 ([Google Patents](https://patents.google.com/patent/US7601126)); its European
counterpart, EP 2043519, is no longer in force. A United States patent; whether it touches what
Strelka does is for its claims to say, should anyone sell sets built from it.
