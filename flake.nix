{
  description = "octos";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
  };
  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    system = "aarch64-darwin";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs {
        inherit system;
      };
    in
      pkgs.mkShellNoCC {
        packages = with pkgs; [
          gcc-arm-embedded
          cmake
          openocd
          zsh
        ];
        shellHook = ''
          exec zsh
        '';
      };
  };
}
