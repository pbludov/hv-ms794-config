$packageName = 'hv-ms794-config'
$installerType = 'msi'
$url = 'https://github.com/pbludov/hv-ms794-config/releases/download/v1.0.0/hv-ms794-config.msi'
$silentArgs = '/Q'
$validExitCodes = @(0)
$checksum = 'TODO'
$checksumType = 'sha256'

Install-ChocolateyPackage "$packageName" "$installerType" "$silentArgs" "$url"  -validExitCodes $validExitCodes -Checksum $checksum -ChecksumType $checksumType
