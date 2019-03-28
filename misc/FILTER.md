## Internal Algorithms & Math

### Filter Types

#### Biquadratic
Specifically, 2nd order (i.e. sos)

 - Implemented using direct form II.
   [Reference](www.earlevel.com/main/2003/02/28/biquads)

 - Nominally 3 feed-forward (A) and 3 feed-back (B) coefficients,
   here normalized to B0 = 1 so only B1, B2 stored.

 - Calculation formulas not currently built-in.
   [Old implementation](github.com/nesanter/xo-gen)
   Derived from both
   [Earlevel](www.earlevel.com/main/2011/01/02/biquad-formulas)
   and [cookbook](shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html).

#### State-Variable
[Reference](www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter)

Code implemented to match block diagram. Support *over* for improved HF
and *order* for easy Linkwitz-Riley style Butterworth cascades (e.g. LR4.)

