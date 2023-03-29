# noman

A command-line tool for accessing personalized notes and cheat sheets instantly.

## Installation

### Building from source

After cloning this repository, you can install `noman` by running the following commands:

```bash
make
sudo make install
```

## Usage

`noman` assumes you have a notes directory (it uses `~/.noman` by default). This directory stores all your notes and cheat sheets. Imagine you have a note called `git.md` inside that directory, you can access it quickly by running:

```bash
noman git
```

Or by using a custom notes directory:

```bash
noman -d ~/mynotes git
```

## Contributing

We welcome any contributions to this library! If you want to contribute to this project, please take a look at the contributing guidelines to see how you can help.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
