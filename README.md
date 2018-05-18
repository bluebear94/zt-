## ztš

> Modern Rymakonian for Zašta, the Luminary of [name unkown]. He was not
> the only one, but he was the first.

> Stop trying to be the next iso
>
> ~ Mareck

A sound change applier with the eventual goal of being able to express the
[sound changes](https://github.com/bluebear94/uruwi-conlangs/raw/master/out/7_1_1.pdf)
from Middle Rymakonian to Modern Rymakonian.

### Building

This project uses CMake. Type `cmake .` then `make` if you're on a
non-retarded operating system.

(Protip: `cmake . -DCMAKE_BUILD_TYPE=Debug` builds a debug build and
`cmake . -DCMAKE_BUILD_TYPE=Release` builds a release build.)

### Usage

    sca_e_kozet <input.zt> <words.txt>

`<input.zt>` is a script in the ztš language. `<words.txt>` is a list of words
separated by newlines. If a line has a `#`, then the part after the `#` will
be passed as the *part-of-speech* parameter, leaving the part before the
symbol as the actual word.

### The ztš language

#### Synopsis

    class C = p f m t s l n k x ŋ h;
    class V = a e i o u;

    feature pa {
      lb: p f m;
      av: t s l n;
      ve: k x ŋ;
      gl: h;
    }

    feature ma {
      pl: p t k;
      fr: f s x h;
      la: l;
      na: m n ŋ;
    }

    # Try replacing the thing after the slash with any of the following:
    # ltr rtl once loopnsi loopsi
    $(C:1|pa=lb) -> $(C:1|pa=av) (_ e) / loopsi;
    s -> ss;
    a -> o (_ ~);
    pp -> p;
    -> e (~ _ ŋ);
    e -> (k _ t);
    -> i (t _ ~);

    # This is an invalid rule, and ztš used to crash on it,
    # but now the validator catches it!
    # $(C:1|pa=lb) -> $(C:2|pa=av);

    # Try to make a velar lateral
    # This doesn't crash but you'll be warned.
    $(C:1|pa=ve) -> $(C:1|ma=la) (_ ~);

    # An example of replacing when the environment is mismatched.
    e -> o !(t _ t);

    # Enumerating matcher
    $(C:1/p,k) -> $(C:1/k,p) / loopsi;

#### Comments

They start with `#` and last until the end of the line.

#### Class declarations

    class <class-name> = <phoneme+>;

This will declare a class with the name `<class-name>` and make it include
the phonemes that follow.

For now, a given phoneme can be in only one class.

#### Feature declarations

    feature <feature-name> [*] {
      <feature-instance>: <phoneme+>;
      # ...
    }

This will declare a feature with the given name. Phonemes take one of the
instances for each feature. Phonemes not listed in a declaration for a
given feature will default to the first on the list.

If `*` is present after the feature name, then the feature is marked as
a non-core feature. This means that this feature will not factor into
the identity of a phoneme.

#### Sound changes:

    <α> -> <ω> [[!] (<λ> _ <ρ>)];

Specify a replacement from `<α>` to `<ω>`, optionally restricting the change
to occur after `<λ>` and before `<ρ>`. Each of the strings can consist of
zero or more of the following:

* a literal (no space needed between two literals; ztš will take the longest
  match to the phonemes it knows currently, or otherwise take one Unicode
  codepoint). Note that you can't use keywords such as `feature` or `class`
  as a string (space them out or wrap them in quotes). I don't want to make
  the lexer context-sensitive.
* a matcher
* in `<λ>` or `<ρ>`, a word boundary marker (`~`) is also allowed at the
  beginning or end, respectively

If the `!` is present, then the rule applies when the environment is not
matched.

Matchers take the following syntax:

    $(<class>[:<number>][|<constraint>(,<constraint>)*])
    $(<class>[:<number>][/<phoneme>(,<phoneme>)*])

The `<number>` tag must be at least 1 if specified (0 is reserved for the
default). Matchers with a `<number>` and without one cannot be mixed with
each other.

Constraints are written as `<feature>=<instance>`. In this case, the matcher
will match only phonemes that have the correct instance of a feature. If
there are multiple constraints, then all of them must be matched to return
a match.

Enumerating matchers are also supported. Note that an enumerating matcher
can backreference only to another enumerating matcher with the same number
of phonemes listed, in which case the phoneme with the same index as the
match is selected.

Of course, matchers can be specified in `<ω>` as well. Such a matcher outputs
the phoneme matched by the same matcher in one of `<α>`, `<λ>` or `<ρ>`
and changes features based on the constraints given. Only core features can
be set in `<ω>`; it wouldn't make sense to change a non-core feature there.

A compound rule can be specified as follows:

    {
      <simple rule>;
      # ...
    };

Note that the final semicolon is necessary. This rule will, for each candidate
position, try each rule in sequence before moving onto the next position.

Finally, options can be passed into sound changes, right before the final
semicolon:

    [/ <option-name+>] [: <pos+>]

`/ <option-name+>` supports the following options:

* `ltr` (default): Match from left to right.
* `rtl`: Match from right to left.
* `once` (default): Replace only once.
* `loopnsi`: Replace as many times as possible, without intersecting matches.
* `loopsi`: Replace as many times as possible, with intersecting matches.

`: <pos+>` allows you to specify which parts of speech to perform this rule
on. Only words with one of the part-of-speech declarations listed will be
affected. This is useful for said 7_1→7_1_1 sound changes, wherein l-recession
depends on the part of speech.

Note that in a compound rule, these options can only be applied to the whole
sound change and not its individual components.

#### Unimplemented features

* The `[<Γ>]` you love from UDN is not supported yet. I'll probably embed
  a scripting language to support this in the future.
* Ordered features (desirable for Middle Rymakonian phonorun reduction) are
  not yet supported.
* Heck, why not add looping rules and such?
* Disjunction in environments is not yet supported. (Probably want this for
  all of `<α>`, `<λ>` and `<ρ>`.)
* Disjunction in constraints is not yet supported.
* Strings can contain only alphabetic and Unicode characters at this moment.
