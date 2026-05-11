# Contributing

## 1. Introduction

This document is for developers who want to modify or contribute to Nano Kernel 
source.

All developers are obligated to read and understand the documentats in `documentation` directory 
which are prefixed with `nk_devel_` prefix:
- `nk_devel_coding_style.md` - [Coding style document](documentation/nk_devel_coding_style.md).
- `nk_devel_pull_requests.md` - [pull requests (PR)](documentation/nk_devel_pull_requests.md).

---
## 2. Vision
Some of key principles used in development of Neon Kit:
- Do not offer a plenthora of source code configuration options. Offering too many options may 
  confuse the user and bring in necessary burden on them to think about configuration (see 
  [wiki overchoice](https://en.wikipedia.org/wiki/Overchoice)). Always choose and hardcode the 
  options which will be of benefit to most of users.
- Always choose the simplest implementation. Use the KISS principle.
- Prefer that a component of a system should behave in a way that most users will expect it to 
  behave (see 
  [wiki principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment)).
- The software is optimized to run on 32-bit microcontrollers.
