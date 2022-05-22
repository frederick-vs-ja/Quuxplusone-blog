---
layout: post
title: "_Heavy Planet_"
date: 2022-05-22 00:01:00 +0000
tags:
  litclub
  math
  this-should-exist
---

The other month I read, in quick succession, two sci-fi classics
involving heavier-than-Earth gravities: Robert Forward's
[_Dragon's Egg_](https://amzn.to/3MDOxSZ) (1980) and Hal Clement's
[_Heavy Planet_](https://amzn.to/3G6K9JH) (comprising four novellas, 1953–1971).

_Dragon's Egg_ is set on the surface of a neutron star, with a surface gravity
equivalent to 67 _billion_ Earth gravities, a rotational period of 0.2 seconds,
and an "organic chemistry" based on the strong nuclear force rather than
electromagnetic bonds. The native life-forms operate at such a rapid pace that
multiple generations can rise and fall in an hour, the entire history of life
is compressed into 5000 years, and the "modern history" of the cheela civilization —
from the invention of agriculture to interstellar spaceflight — takes place over
about a month (thanks to an _extremely_ coincidental meeting with an expedition
from Earth at just the right moment in their history).

_Dragon's Egg_ is often compared to Hal Clement's 1953 _Mission of Gravity_
(collected in _Heavy Planet_), in that both novels involve human contact with
the inhabitants of a rapidly whirling, super-massive world. _Heavy Planet_, though,
involves numbers on a much more human scale: its planet [Mesklin](https://en.wikipedia.org/wiki/Mesklin)
is merely 5000 times the mass of Earth, with a rotational period of about 18 minutes.
This gives Mesklin a surface gravity of... well, it depends on your latitude!
Near the equator, 38600 km from the planet's center, Mesklin's rapid whirling
(227 km/s angular velocity) produces a centrifugal acceleration of v²/r = 1330 m/s²,
canceling out most of the 1360 m/s² acceleration due to gravity at that distance.
The result is that gravity at Mesklin's equator is only three times that of Earth;
but at the poles, where the centrifugal acceleration is nil and (because Mesklin is
an _extremely_ oblate spheroid with a polar radius of only 16000 km) we're talking
something like 275 Earth gravities. (My naïve arithmetic says 800 gravities, but
my understanding is that it's not terribly appropriate to approximate Mesklin as a point
mass: at the pole, much of Mesklin's equatorial mass is actually "above" you in its
gravity well, producing a countervailing gravitational force. The 275-_g_ number comes
from a computer program written by the [MIT Science Fiction Society](https://en.wikipedia.org/wiki/MIT_Science_Fiction_Society)
back in 1953-something.) So, in Clement's stories, the chemistry is ordinary organic
chemistry, and human astronauts can even visit the surface of the planet and interact
directly with the natives — as long as they stay near the equator!

Gregory Brannon has created [a mod for _Kerbal Space Program_](https://github.com/GregroxMun/Whirligig-World)
that simulates a "whirligig world" similar to Mesklin.

Hal Clement's Mesklinites are technologically about where Earthlings were in the middle of the
second millennium, and the narrative itself is a straightforwardly rollicking exploration-and-adventure
travelogue, following the native ship's captain Barlennan on a voyage similar to those of Captain Cook.
One delightful bit of worldbuilding: Just like Earth, Mesklinite navigation has a
[longitude problem](https://en.wikipedia.org/wiki/History_of_longitude) but no latitude problem, despite
Mesklin's cloudy skies. A Mesklinite sailor needs no sextant to measure latitude; all he needs is a
wooden spring with a weight attached. The further you get from the pole, the higher the weight lifts.

But here's the real reason I'm writing this post: Mesklinite gravity is _so high_ that it's hard
for humans to comprehend. There's [a viral video](https://i.imgur.com/wViz696.mp4) going around
right now that shows a simulated pallet of lumber falling on a car in Earth gravity, Jupiter gravity,
and so on.  Apparently it was made in the driving (and soft-body collision) simulator
[BeamNG.drive](https://www.beamng.com/game/), which has a gravity slider; but according to
[the Reddit comments](https://old.reddit.com/r/Damnthatsinteresting/comments/t9j91e/perception_of_gravity_in_different_celestial/)
the slider maxes out at "Sun gravity" (274 m/s², or about 28 _g_), which is vastly lower than the gravity
at Mesklin's poles (which, remember, MITSFS estimated at 275 _g_ = 2700 m/s²). From _Mission of Gravity_:

> Seeing things fall free in triple gravity, Lackland found, was even worse than
> thinking about it. Maybe it would be better at the poles — then you couldn't see
> them at all. Not where an object falls some two miles in the first second! But
> perhaps the abrupt vanishing would be just as hard on the nerves.

And again, when the crew encounters a three-hundred-foot cliff:

> One of the crew [...] rolled a bullet-sized pebble to the edge of the cliff
> and given it a final shove. The results had been interesting, to both Mesklinites
> and Earthmen. The latter could see nothing, since the only view-set at the foot
> of the cliff was still aboard the _Bree_ and too distant from the point of impact
> to get a distinct view; but they heard as well as the natives. As a matter of fact,
> they saw almost as well; for even to Mesklinite vision the pebble simply vanished.
> There was a short note like a breaking violin string as it clove the air, followed
> a split second later by a sharp report as it struck the ground below.
>
> Fortunately it landed on hard, slightly moist ground rather than on another stone[.]
> The impact, at a speed of approximately a mile a second, sent the ground splashing
> outward in a wave too fast for any eye to see while it was in motion, but which froze
> after a fraction of a second, leaving a rimmed crater surrounding the deeper hole
> the missile had drilled in the soil. Slowly the sailors gathered around, eyeing the
> gently steaming ground; then with one accord they moved a few yards away from the
> foot of the cliff. It took some time to shake off the mood that experiment
> engendered.

(To be fair, when I do this math, I think that either the cliff was 300 _meters_ tall,
or else the pebble was traveling only _half_ a mile a second, or 1600 mph, at impact.
Still.)

I'd love to see some animations of soft-body dynamics under Mesklin-like gravitational
conditions; even better if the model takes into account the massive conversion of
kinetic energy into heat that must follow such an impact. If you know of a way to simulate
and render such an animation, please let me know!
