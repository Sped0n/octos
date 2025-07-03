{
  description = "octos";
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
        name = "octos";
        packages = with pkgs; [
          gcc-arm-embedded
          cmake
          openocd
        ];
      };
  };
}
