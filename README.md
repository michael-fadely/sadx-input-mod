# sadx-input-mod
sadx-input-mod is a mod for SADX PC that replaces the default DirectInput polling with SDL2 to enable more robust gamepad support. It allows the use of DirectInput and XInput controllers simultaneously.

### What it can do
* Configurable deadzones per analog stick
* Configurable rumble intensity
* Per-controller rumble (vanilla SADX only allows rumble for P1's controller)
* Fixed right analog stick (first person camera bug) for XInput and some DirectInput controllers
* Enables extended Dreamcast buttons C, D, and Z, mapped to LB (PSx L1), Back (PS1-PS3 Select, PS4 Share), and RB (PSx R1) respectively.

### What it *can't* do
* Configurable controls (SDL controller maps work though!)

## Configuration
To configure each controller, create a file called `config.ini` in the mod's root folder (`mods/sadx-input-mod`).

### `[Controller N]` section
Where `N` is the controller slot to configure. In official builds, you can use and configure up to 8 controllers.
Configurable controller fields are as follows:

#### Fields
| Field                    | Type        | Range        | Default     | Description |
| ------------------------ | ----------- | ------------ | -----------:| ----------- |
| `DeadzoneL`              | integer     | `0`, `32767` |      `7849` | Analog stick deadzone. |
| `DeadzoneR`              | integer     | `0`, `32767` |      `8689` | Analog stick deadzone. |
| `RadialL`                | boolean     | `0`, `1`     |         `1` | Enables full range of motion on this analog stick. |
| `RadialR`                | boolean     | `0`, `1`     |         `0` | Ditto. If not explicitly configured and [sadx-smooth-cam](https://github.com/SonicFreak94/sadx-smooth-cam) is detected, it is `1` by default. |
| `TriggerThreshold`       | integer     | `0`, `32767` |        `30` | Trigger to digital conversion threshold. |
| `RumbleFactor`           | float       | `0.0`, `1.0` |       `1.0` | Rumble multiplier. Values below 0 or above 1 have no effect. |
| `MegaRumble`             | boolean     | `0`, `1`     |         `0` | Always fire both motors while rumbling, never independently. |

#### Example
```ini
[Controller 1]
DeadzoneL = 4096
DeadzoneR = 4096
RadialR = 1
```

### `[Config]` section
This section allows you to enable ingame display debugging for all controllers.

#### Fields
| Field                    | Type        | Range        | Default     | Description |
| ------------------------ | ----------- | ------------ | -----------:| ----------- |
| `Debug`                  | boolean     | `0`, `1`     |         `0` | Enables ingame controller debugging information. |

#### Example
```ini
[Config]
Debug = 1
```
