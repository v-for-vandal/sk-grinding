using Mutagen.Bethesda;
using Mutagen.Bethesda.Archives;
internal static class Sources {
 public static void Extract(string[] args) {
    var output=Path.GetFullPath(".tools/papyrus-sources");Directory.CreateDirectory(output);
    foreach(var path in args) {
        var archive=Archive.CreateReader(GameRelease.SkyrimSE,path);
        foreach(var file in archive.Files) if(file.Path.EndsWith(".psc",StringComparison.OrdinalIgnoreCase)||file.Path.EndsWith(".flg",StringComparison.OrdinalIgnoreCase)) {
            var name=Path.GetFileName(file.Path.Replace('\\','/'));
            File.WriteAllBytes(Path.Combine(output,name),file.GetBytes());
        }
    }
    Console.WriteLine($"Extracted script sources to {output}");
 }
}
