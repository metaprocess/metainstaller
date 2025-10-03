import { useState } from 'react'
import {
  useDetailedContainers,
  useStartContainer,
  useStopContainer,
  useRestartContainer,
  usePauseContainer,
  useUnpauseContainer,
  useRemoveContainer,
  useContainerLogs
} from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Table, TableBody, TableCell, TableHead, TableHeader, TableRow, Input } from '@/components/ui'
import { Badge, Alert, AlertDescription } from '@/components/ui'
import { Container, Play, Square, RotateCcw, Pause, Trash2, FileText, Search, RefreshCw, Filter } from 'lucide-react'
import type { Container as ContainerType } from '@/types/api'

const ContainerManagement = () => {
  const [showAllContainers, setShowAllContainers] = useState(true)
  const [searchTerm, setSearchTerm] = useState('')
  const [selectedContainer, setSelectedContainer] = useState<string | null>(null)

  const { data: containers, refetch, isLoading } = useDetailedContainers(showAllContainers)
  const { data: logs } = useContainerLogs(selectedContainer || '', 100)
  
  const startContainer = useStartContainer()
  const stopContainer = useStopContainer()
  const restartContainer = useRestartContainer()
  const pauseContainer = usePauseContainer()
  const unpauseContainer = useUnpauseContainer()
  const removeContainer = useRemoveContainer()

  const handleAction = async (action: string, containerId: string) => {
    try {
      switch (action) {
        case 'start':
          await startContainer.mutateAsync(containerId)
          break
        case 'stop':
          await stopContainer.mutateAsync({ containerId })
          break
        case 'restart':
          await restartContainer.mutateAsync({ containerId })
          break
        case 'pause':
          await pauseContainer.mutateAsync(containerId)
          break
        case 'unpause':
          await unpauseContainer.mutateAsync(containerId)
          break
        case 'remove':
          if (confirm('Are you sure you want to remove this container?')) {
            await removeContainer.mutateAsync({ containerId })
          }
          break
        default:
          console.warn('Unknown action:', action)
      }
      refetch()
    } catch (error) {
      console.error('Action failed:', error)
    }
  }

  const getStatusBadge = (status: string) => {
    if (status.toLowerCase().includes('up') && status.toLowerCase().includes('paused')) {
      return <Badge variant="warning">Paused</Badge>
    } else if (status.toLowerCase().includes('up')) {
      return <Badge variant="success">Running</Badge>
    } else if (status.toLowerCase().includes('exited')) {
      return <Badge variant="secondary">Exited</Badge>
    
    } else {
      return <Badge variant="destructive">Unknown</Badge>
    }
  }

  const isContainerRunning = (status: string) => {
    return (status.toLowerCase().includes('up') && !status.toLowerCase().includes('paused'))
  }

  const isContainerPaused = (status: string) => {
    return (status.toLowerCase().includes('up') && status.toLowerCase().includes('paused'))
  }

  const filteredContainers = containers?.containers.filter((container: ContainerType) =>
    container.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
    container.image.toLowerCase().includes(searchTerm.toLowerCase()) ||
    container.id.toLowerCase().includes(searchTerm.toLowerCase())
  ) || []

  const runningCount = containers?.containers.filter((c: ContainerType) => 
    isContainerRunning(c.status)).length || 0
  const stoppedCount = containers?.containers.filter((c: ContainerType) => 
    !isContainerRunning(c.status)).length || 0

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold">Container Management</h1>
          <p className="text-gray-600 mt-2">Manage Docker containers and their lifecycle</p>
        </div>
        <Button onClick={() => refetch()} disabled={isLoading}>
          <RefreshCw className="h-4 w-4 mr-2" />
          Refresh
        </Button>
      </div>

      {/* Stats */}
      <div className="grid gap-4 md:grid-cols-3">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Total Containers</CardTitle>
            <Container className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{containers?.containers.length || 0}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Running</CardTitle>
            <Play className="h-4 w-4 text-green-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold text-green-600">{runningCount}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Stopped</CardTitle>
            <Square className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold text-gray-600">{stoppedCount}</div>
          </CardContent>
        </Card>
      </div>

      {/* Filters and Search */}
      <Card>
        <CardContent className="p-4">
          <div className="flex flex-col sm:flex-row gap-4">
            <div className="relative flex-1">
              <Search className="absolute left-3 top-3 h-4 w-4 text-gray-400" />
              <Input
                placeholder="Search containers by name, image, or ID..."
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
                className="pl-9"
              />
            </div>
            <Button
              variant={showAllContainers ? "primary" : "outline"}
              onClick={() => setShowAllContainers(!showAllContainers)}
            >
              <Filter className="h-4 w-4 mr-2" />
              {showAllContainers ? 'Show Running Only' : 'Show All'}
            </Button>
          </div>
        </CardContent>
      </Card>

      {/* Containers Table */}
      <Card>
        <CardHeader>
          <CardTitle>Containers</CardTitle>
          <CardDescription>
            {filteredContainers.length} containers found
          </CardDescription>
        </CardHeader>
        <CardContent>
          {filteredContainers.length === 0 ? (
            <div className="text-center py-8">
              <Container className="h-12 w-12 text-gray-400 mx-auto mb-4" />
              <p className="text-gray-500">No containers found</p>
            </div>
          ) : (
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Name</TableHead>
                  <TableHead>Image</TableHead>
                  <TableHead>Status</TableHead>
                  <TableHead>Created</TableHead>
                  <TableHead>Ports</TableHead>
                  <TableHead>Actions</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {filteredContainers.map((container: ContainerType) => (
                  <TableRow key={container.id}>
                    <TableCell className="font-medium">
                      <div>
                        <div className="font-medium">{container.name}</div>
                        <div className="text-xs text-gray-500">
                          {container.id.substring(0, 12)}
                        </div>
                      </div>
                    </TableCell>
                    <TableCell>
                      <div className="max-w-xs" title={container.image}>
                        {container.image}
                      </div>
                    </TableCell>
                    <TableCell>
                      {getStatusBadge(container.status)}
                      <div className="text-xs text-gray-500 mt-1">
                        {container.status}
                      </div>
                    </TableCell>
                    <TableCell className="text-sm">{container.created}</TableCell>
                    <TableCell className="text-sm">
                      {Array.isArray(container.ports) ? 
                        container.ports.length > 0 ? container.ports.join(', ') : 'None' :
                        container.ports || 'None'
                      }
                    </TableCell>
                    <TableCell>
                      <div className="flex space-x-1">
                        {!isContainerRunning(container.status) && !isContainerPaused(container.status) && (
                          <Button
                            size="sm"
                            variant="outline"
                            onClick={() => handleAction('start', container.id)}
                            disabled={startContainer.isPending}
                          >
                            <Play className="h-3 w-3" />
                          </Button>
                        )}
                        
                        {isContainerRunning(container.status) && (
                          <>
                            <Button
                              size="sm"
                              variant="outline"
                              onClick={() => handleAction('stop', container.id)}
                              disabled={stopContainer.isPending}
                            >
                              <Square className="h-3 w-3" />
                            </Button>
                            <Button
                              size="sm"
                              variant="outline"
                              onClick={() => handleAction('pause', container.id)}
                              disabled={pauseContainer.isPending}
                            >
                              <Pause className="h-3 w-3" />
                            </Button>
                          </>
                        )}

                        {isContainerPaused(container.status) && (
                          <Button
                            size="sm"
                            variant="outline"
                            onClick={() => handleAction('unpause', container.id)}
                            disabled={unpauseContainer.isPending}
                          >
                            <Play className="h-3 w-3" />
                          </Button>
                        )}
                        
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => handleAction('restart', container.id)}
                          disabled={restartContainer.isPending}
                        >
                          <RotateCcw className="h-3 w-3" />
                        </Button>
                        
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => setSelectedContainer(
                            selectedContainer === container.id ? null : container.id
                          )}
                        >
                          <FileText className="h-3 w-3" />
                        </Button>
                        
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => handleAction('remove', container.id)}
                          disabled={removeContainer.isPending}
                          className="text-red-600 hover:text-red-700"
                        >
                          <Trash2 className="h-3 w-3" />
                        </Button>
                      </div>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          )}
        </CardContent>
      </Card>

      {/* Container Logs */}
      {selectedContainer && logs && (
        <Card>
          <CardHeader>
            <CardTitle>Container Logs</CardTitle>
            <CardDescription>
              Showing logs for container {selectedContainer.substring(0, 12)}
            </CardDescription>
          </CardHeader>
          <CardContent>
            <pre className="text-xs bg-gray-50 p-4 rounded border overflow-auto max-h-96 whitespace-pre-wrap">
              {logs.logs || 'No logs available'}
            </pre>
          </CardContent>
        </Card>
      )}
    </div>
  )
}

export default ContainerManagement