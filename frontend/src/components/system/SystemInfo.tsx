import { useSystemInfo, useDiskUsage, usePruneSystem } from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui'
import { Alert, AlertDescription } from '@/components/ui'
import { Server, HardDrive, Cpu, MemoryStick, RefreshCw, Trash2, AlertCircle } from 'lucide-react'

const SystemInfo = () => {
  const { data: systemInfo, refetch: refetchSystemInfo, isLoading } = useSystemInfo()
  const { data: diskUsage, refetch: refetchDiskUsage } = useDiskUsage()
  const pruneSystem = usePruneSystem()

  const handlePruneSystem = async () => {
    if (confirm('Are you sure you want to clean up unused Docker data? This will remove unused containers, images, networks, and volumes.')) {
      try {
        await pruneSystem.mutateAsync()
        refetchDiskUsage()
        refetchSystemInfo()
      } catch (error) {
        console.error('Prune failed:', error)
      }
    }
  }

  const formatBytes = (bytes: number): string => {
    if (bytes === 0) return '0 Bytes'
    const k = 1024
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
    const i = Math.floor(Math.log(bytes) / Math.log(k))
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
  }

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold">System Information</h1>
          <p className="text-gray-600 mt-2">Docker system overview and maintenance</p>
        </div>
        <div className="flex space-x-2">
          <Button 
            onClick={handlePruneSystem}
            variant="destructive"
            loading={pruneSystem.isPending}
          >
            <Trash2 className="h-4 w-4 mr-2" />
            Cleanup System
          </Button>
          <Button 
            onClick={() => {
              refetchSystemInfo()
              refetchDiskUsage()
            }} 
            disabled={isLoading}
          >
            <RefreshCw className="h-4 w-4 mr-2" />
            Refresh
          </Button>
        </div>
      </div>

      {pruneSystem.isSuccess && (
        <Alert variant="success">
          <AlertDescription>
            <div className="flex items-center space-x-2">
              <AlertCircle className="h-4 w-4" />
              <span>System cleanup completed successfully.</span>
            </div>
          </AlertDescription>
        </Alert>
      )}

      {!pruneSystem.error ? null : (
        <Alert variant="destructive">
          <AlertDescription>
            <div className="flex items-center space-x-2">
              <AlertCircle className="h-4 w-4" />
              <span>System cleanup failed. Please check your permissions.</span>
            </div>
          </AlertDescription>
        </Alert>
      )}

      {/* System Overview */}
      <div className="grid gap-6 md:grid-cols-2 lg:grid-cols-4">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Containers</CardTitle>
            <Server className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{systemInfo?.Containers || 0}</div>
            <div className="text-xs text-gray-500 mt-1">
              {systemInfo?.ContainersRunning || 0} running, {systemInfo?.ContainersStopped || 0} stopped
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Images</CardTitle>
            <HardDrive className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{systemInfo?.Images || 0}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">CPU Cores</CardTitle>
            <Cpu className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{systemInfo?.NCPU || 0}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Memory</CardTitle>
            <MemoryStick className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {systemInfo?.MemTotal ? formatBytes(systemInfo.MemTotal) : 'N/A'}
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Disk Usage */}
      <Card>
        <CardHeader>
          <CardTitle>Disk Usage</CardTitle>
          <CardDescription>Docker disk usage by resource type</CardDescription>
        </CardHeader>
        <CardContent>
          {(diskUsage && diskUsage.length > 0) ? (
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Type</TableHead>
                  <TableHead>Total Count</TableHead>
                  <TableHead>Active</TableHead>
                  <TableHead>Size</TableHead>
                  <TableHead>Reclaimable</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {diskUsage.map((item, index) => (
                  <TableRow key={index}>
                    <TableCell className="font-medium">{item.Type}</TableCell>
                    <TableCell>{item.TotalCount}</TableCell>
                    <TableCell>{item.Active}</TableCell>
                    <TableCell>{item.Size}</TableCell>
                    <TableCell>{item.Reclaimable}</TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          ) : (
            <div className="text-center py-4 text-gray-500">
              No disk usage data available
            </div>
          )}
        </CardContent>
      </Card>

      {/* System Details */}
      <div className="grid gap-6 lg:grid-cols-2">
        <Card>
          <CardHeader>
            <CardTitle>Docker Information</CardTitle>
            <CardDescription>Core Docker system details</CardDescription>
          </CardHeader>
          <CardContent>
            {systemInfo ? (
              <div className="space-y-3 text-sm">
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Version:</span>
                  <span>{systemInfo.ServerVersion}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">API Version:</span>
                  <span>{'N/A'}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Driver:</span>
                  <span>{systemInfo.Driver}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Logging Driver:</span>
                  <span>{systemInfo.LoggingDriver}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Cgroup Driver:</span>
                  <span>{systemInfo.CgroupDriver}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Default Runtime:</span>
                  <span>{systemInfo.DefaultRuntime}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Docker Root Dir:</span>
                  <span className="break-all">{systemInfo.DockerRootDir}</span>
                </div>
              </div>
            ) : (
              <div className="text-center py-4 text-gray-500">
                No system information available
              </div>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Host System</CardTitle>
            <CardDescription>Host system information</CardDescription>
          </CardHeader>
          <CardContent>
            {systemInfo ? (
              <div className="space-y-3 text-sm">
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Operating System:</span>
                  <span>{systemInfo.OperatingSystem}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">OS Type:</span>
                  <span>{systemInfo.OSType}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Architecture:</span>
                  <span>{systemInfo.Architecture}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Kernel Version:</span>
                  <span>{systemInfo.KernelVersion}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Hostname:</span>
                  <span>{systemInfo.Name}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">System Time:</span>
                  <span>{new Date(systemInfo.SystemTime).toLocaleString()}</span>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <span className="text-gray-500">Goroutines:</span>
                  <span>{systemInfo.NGoroutines}</span>
                </div>
              </div>
            ) : (
              <div className="text-center py-4 text-gray-500">
                No host information available
              </div>
            )}
          </CardContent>
        </Card>
      </div>

      {/* Resource Limits */}
      <Card>
        <CardHeader>
          <CardTitle>Resource Limits</CardTitle>
          <CardDescription>Available Docker resource limits and features</CardDescription>
        </CardHeader>
        <CardContent>
          {systemInfo ? (
            <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4 text-sm">
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.MemoryLimit ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>Memory Limit</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.SwapLimit ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>Swap Limit</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.KernelMemoryTCP ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>Kernel Memory TCP</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.CpuCfsPeriod ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>CPU CFS Period</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.CpuCfsQuota ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>CPU CFS Quota</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.CPUShares ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>CPU Shares</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.CPUSet ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>CPU Set</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.PidsLimit ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>PIDs Limit</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.IPv4Forwarding ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>IPv4 Forwarding</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.OomKillDisable ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>OOM Kill Disable</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.Debug ? 'bg-yellow-500' : 'bg-gray-400'}`}></div>
                <span>Debug Mode</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${systemInfo.LiveRestoreEnabled ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span>Live Restore</span>
              </div>
            </div>
          ) : (
            <div className="text-center py-4 text-gray-500">
              No resource limit information available
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  )
}

export default SystemInfo