## ztš

**ALERT:** Moved to [GitLab](https://gitlab.com/Kyarei/zt).

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

You need Boost (for `filesystem`) installed as well.

`make atest` runs the tests. If you're hacking on ztš, then:

* make sure to run the tests whenever you change the code
* make sure you add tests for new features

### Usage

Run the program with literally anything that isn't a valid input to see the
usage for the command.

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
    $(C:1|pa!=ve) -> $(C:1|ma=na) (_ ŋ);

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

#### String literals

A *string literal* is:

* a series of bytes that are either letters in ASCII or non-ASCII bytes
  (thereby allowing Unicode characters to be put in such a place), other than
  the keywords `feature`, `class`, `NOT`, `ordered`, `executeOnce` or `setOptions`
* a series of characters other than `\` or `"`, or the escape sequences
  `\\`, `\"` or `\n` (meaning what you expect them to mean), surrounded by
  double quotes

These are used in a lot of places, included as class or feature names,
feature instances and phonemes.

Note that sound change option names are not reserved words. For instance,
it's perfectly legal to name a feature `ltr` or `loopsi`.

#### Class declarations

    class <class-name> = <phoneme+>;

This will declare a class with the name `<class-name>` and make it include
the phonemes that follow.

For now, a given phoneme can be in only one class.

#### Feature declarations

    feature <feature-name> [*] [ordered] {
      <feature-instance> [*]: <phoneme+>;
      # ...
    }

This will declare a feature with the given name. Each phoneme takes one of the
*instances* of each feature.

If `*` is present after the feature name, then the feature is marked as
a non-core feature. This means that this feature will not factor into
the identity of a phoneme.

If the `ordered` keyword is provided, then the feature is *ordered*. That is,
the comparisons `<`, `>`, `<=` and `>=` can be used on it. Feature instances
appearing lower in the feature definition will be "greater".

If `*` is present after a feature *instance* name, then that one is set as
the default – that is, any phonemes not listed will take that instance. At
most one instance can be set as the default.

In addition to explicitly listing phonemes, you can also import all phonemes
that have a certain instance of a feature:

    feature(<feature-name>=<feature-instance+>)

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
* an alternation: `a|b|c` will match either `a` or `b` or `c`.
* any of the above, plus a repetition modifier:
  * `*` matches zero or more
  * `?` matches zero or one
  * `+` matches one or more
  * `{<m>}` matches `<m>` exactly
  * `{<m>,}` matches `<m>` or more
  * `{<m>, <n>}` matches at least `<m>` but at most `<n>`

If the `!` is present, then the rule applies when the environment is not
matched.

Matchers take the following syntax:

    $(<class>[:<number>][|<constraint>(,<constraint>)*])
    $(<class>[:<number>][/<phoneme>(,<phoneme>)*])

The `<number>` tag must be at least 1 if specified (0 is reserved for the
default). Matchers with a `<number>` and without one cannot be mixed with
each other.

Constraints are written as `<feature> <op> <instance+>`, where:

* `<feature>` is a feature name, obviously
* `<op>` is either `=` or `!=` (only `=` is allowed in `<ω>`); if `<feature>`
  is ordered, then `<`, `>`, `<=` and `>=` are also allowed
* `<instance+>` is a list of one or more instances of the feature you want
  to match for (or against). That is, if `<op>` is `=`, then the constraint
  will match phonemes with that feature set to any of the instances listed.
  If `<op>` is `!=`, then the constraint will match the phonemes with that
  feature set to a value other than any of the instances listed. For instance,
  `pa=lb av` will match phonemes with `pa` set to `lb` or `av`, while
  `pa!=lb av` will match phonemes with `pa` set to anything else.
  * An `<instance>` can either be a single feature instance name or refer
    to the value taken by another matcher: for instance, `$(C:2|pa=C:1)`
    means "match a consonant with the same `pa` as whatever `C:1` matched".
  * `<`, `>`, `<=` and `>=` act differently when given multiple instances
    to match against. They match only when the comparison succeeds for all
    of the values. (Should this be changed?)

In this case, the matcher will match only phonemes that have the correct
instance of a feature. If there are multiple constraints, then all of them
must be matched to return a match.

Enumerating matchers are also supported. Note that an enumerating matcher
can backreference only to another enumerating matcher with the same number
of phonemes listed, in which case the phoneme with the same index as the
match is selected. *NB: enumerating matchers can be thought of as a special
case of alternation, but faster (no backing up the list of matcher matches)
and legal in `<ω>`.*

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

You can also change the default options using `setOptions <option-name+>;`.

`: <pos+>` allows you to specify which parts of speech to perform this rule
on. Only words with one of the part-of-speech declarations listed will be
affected. This is useful for said 7_1→7_1_1 sound changes, wherein l-recession
depends on the part of speech.

Note that in a compound rule, these options can only be applied to the whole
sound change and not its individual components.

#### Nitty-gritties of matching

`<α>`, `<ρ>` and `<ω>` are matched left-to-right in an `/ ltr` sound change
and right-to-left in an `/ rtl` sound change. `<λ>` is matched in the opposite
direction as the others.

In an `/ ltr` sound change, `<α>` is checked first, then `<λ>`, then `<ρ>`,
then the substitution to `<ω>` is performed. In an `/ rtl` sound change,
the order `<α>` → `<ρ>` → `<λ>` → `<ω>`.

A matcher captured inside an alternation is considered "seen" outside the
alternation if it is "seen" in all of its branches. Note that this is just
how the static verifier checks things. For instance, in the following string:

    [k|$(C:1|ma=pl)]aa$(C:1|ma=fr)b

the static verifier considers `$(C:1)` seen at the `b` (assuming left-to-right
checking), but when the rule is actually run, `$(C:1)` could be captured
earlier.

#### Lua scripting

Lua code blocks are surrounded by `$$` (two dollar signs on each side) around
them. Funky things will happen if you happen to have that string within
your Lua code.

*Global* Lua code blocks are run once during the invocation of `ztš`. The
syntax is `executeOnce <lua_code>`; for instance, if you want to print a
string once in a program, insert the following:

    executeOnce $$
    print("soonoyun i lua!")
    $$

Of course, the real magic comes when you make rules fire only when a certain
condition is met. This is the infamous Γ from UDN. Just pop a Lua code block
right after the environment (if you have one):

    a+ -> "OKITA-SAN DAISHOURI!" (~ _ ~) $$ isPrime(M.n) $$;

Given a string with *n* `a`s (and nothing else), this rule replaces it with
`OKITA-SAN DAISHOURI!` if *n* is prime (given a suitable definition of
`isPrime`, of course). Otherwise, the substitution is not done:

    aaaaaaa -> OKITA-SAN DAISHOURI!
    aaaaaaaa -> aaaaaaaa

The following variables are available inside Γ-expressions:

* `W`: the word as it is before the rule is applied. A list of *phoneme spec*
  objects, one-indexed. (I personally dislike one-indexing, but that's what
  Lua does.)
* `M`: a table with the following entries:
  * `s`: the index of the first character matched (from one).
  * `e`: the index right after the last character matched (from one).
    That is, `e` can range from `1` to `#W + 1`.
  * `n`: the number of characters matched – `e - s`.

`sca` is available pretty much everywhere and refers to the current SCA
object.

##### `ztš.SCA`

    sca:getPhoneme(name) -- name is a string, returns a phoneme spec object
    sca:getFeature(name) -- name is a string, returns a pair (index, feature)
                         -- index is from zero; feature is a feature object
    sca:getClass(name)   -- similar, but returns char class object as 2nd elem
    sca:getFeatureByIndex(index) -- similar to the two above, but take in the
    sca:getClassByIndex(index)   -- index and return only the relevant object

##### `ztš.SCA.PhonemeSpec`

    ps:getName()           -- returns the name of this phoneme spec
    ps:getCharClass(sca)   -- takes in an SCA object, returns a char class
    ps:getCharClassIndex() -- just returns the index
                           -- (you can feed it into sca:getClassByIndex)
    ps:getFeatureValue(fid, sca)
                           -- fid is a feature index (get from sca:getFeature)
                           -- sca is an sca object
                           -- returns a 1-based index into the
                           -- feature:getInstanceNames() table
    ps:getFeatureName(fid, sca)
                           -- similar, but actually returns the name

##### `ztš.SCA.Feature`

    feature:getName()          -- returns the name
    feature:getInstanceNames() -- returns a table of instance names for
                               -- this feature
    feature:getDefault()       -- default instance, is an index
    feature:isCore()           -- true if core feature
    feature:isOrdered()        -- true if ordered feature

##### `ztš.SCA.CharClass`

    cc:getName() -- returns the name

#### Unimplemented features

* Heck, why not add looping rules and such?
* Disjunction in constraints is not yet supported in general (e. g. it's not
  yet possible to match phonemes with, say, `pa=lb` or `ma=pl`). This can
  probably be done with alternation.
