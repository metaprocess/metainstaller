import {
  useDockerServiceStatus,
  useStartDockerService,
  useStopDockerService,
  useRestartDockerService,
  useEnableDockerService,
  useDisableDockerService
} from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Alert, AlertDescription } from '@/components/ui'
import { Badge } from '@/components/ui'
import { Play, Square, RotateCcw, Power, PowerOff, CheckCircle, XCircle, AlertCircle } from 'lucide-react'

const ServiceManagement = () => {
  const { data: serviceStatus, refetch } = useDockerServiceStatus()
  const startService = useStartDockerService()
  const stopService = useStopDockerService()
  const restartService = useRestartDockerService()
  const enableService = useEnableDockerService()
  const disableService = useDisableDockerService()

  const handleStart = () => {
    startService.mutate(undefined, {
      onSuccess: () => refetch(),
      onError(error, variables, context) {
        alert();
      }
    })
  }

  const handleStop = () => {
    stopService.mutate(undefined, {
      onSuccess: () => refetch()
    })
  }

  const handleRestart = () => {
    restartService.mutate(undefined, {
      onSuccess: () => refetch()
    })
  }

  const handleEnable = () => {
    enableService.mutate(undefined, {
      onSuccess: () => refetch()
    })
  }

  const handleDisable = () => {
    disableService.mutate(undefined, {
      onSuccess: () => refetch()
    })
  }

  const isLoading = startService.isPending || stopService.isPending || restartService.isPending ||
    enableService.isPending || disableService.isPending

  const getStatusIcon = (running: boolean) => {
    return running ? (
      <CheckCircle className="h-5 w-5 text-green-500" />
    ) : (
      <XCircle className="h-5 w-5 text-red-500" />
    )
  }

  const getStatusBadge = (running: boolean) => {
    return (
      <Badge variant={running ? 'success' : 'destructive'}>
        {running ? 'Running' : 'Stopped'}
      </Badge>
    )
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold">Main Service Management</h1>
        <p className="text-gray-600 mt-2">Start, stop, and manage the Main service</p>
      </div>

      {!(
        startService.error || stopService.error || restartService.error ||
        enableService.error || disableService.error
      ) ? null : (
        <Alert variant="destructive">
          <AlertDescription>
            <div className="flex items-center space-x-2">
              <AlertCircle className="h-4 w-4" />
              <span>Operation failed. Please check your permissions and try again.</span>
            </div>
          </AlertDescription>
        </Alert>
      )}

      {(startService.isSuccess || stopService.isSuccess || restartService.isSuccess ||
        enableService.isSuccess || disableService.isSuccess) && (
          <Alert variant="success">
            <AlertDescription>
              <div className="flex items-center space-x-2">
                <CheckCircle className="h-4 w-4" />
                <span>Operation completed successfully.</span>
              </div>
            </AlertDescription>
          </Alert>
        )}

      <div className="grid gap-6 lg:grid-cols-2">
        <Card>
          <CardHeader>
            <div className="flex items-center justify-between">
              <div>
                <CardTitle className="flex items-center space-x-2">
                  {getStatusIcon(serviceStatus?.running || false)}
                  <span>Service Status</span>
                </CardTitle>
                <CardDescription>Current Docker service state</CardDescription>
              </div>
              {getStatusBadge(serviceStatus?.running || false)}
            </div>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="grid gap-3">
              <div className="flex justify-between items-center">
                <span className="text-sm text-gray-600">Service State:</span>
                <span className="font-medium">
                  {serviceStatus?.running ? 'Active (running)' : 'Inactive (stopped)'}
                </span>
              </div>
            </div>

            {serviceStatus?.status && (
              <div className="mt-4">
                <h4 className="text-sm font-medium mb-2">Service Details:</h4>
                <pre className="text-xs bg-gray-50 p-3 rounded border overflow-auto max-h-40">
                  {serviceStatus.status}
                </pre>
              </div>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Service Controls</CardTitle>
            <CardDescription>Manage Docker service lifecycle</CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="grid gap-2">
              <Button
                onClick={handleStart}
                disabled={serviceStatus?.running || isLoading}
                loading={startService.isPending}
                className="justify-start"
              >
                <Play className="h-4 w-4 mr-2" />
                Start Service
              </Button>

              <Button
                variant="destructive"
                onClick={handleStop}
                disabled={!serviceStatus?.running || isLoading}
                loading={stopService.isPending}
                className="justify-start"
              >
                <Square className="h-4 w-4 mr-2" />
                Stop Service
              </Button>

              <Button
                variant="outline"
                onClick={handleRestart}
                disabled={isLoading}
                loading={restartService.isPending}
                className="justify-start"
              >
                <RotateCcw className="h-4 w-4 mr-2" />
                Restart Service
              </Button>
            </div>

            <div className="border-t pt-4">
              <h4 className="text-sm font-medium mb-3">Boot Configuration</h4>
              <div className="grid gap-2">
                <Button
                  variant="outline"
                  onClick={handleEnable}
                  disabled={isLoading}
                  loading={enableService.isPending}
                  className="justify-start"
                >
                  <Power className="h-4 w-4 mr-2" />
                  Enable on Boot
                </Button>

                <Button
                  variant="outline"
                  onClick={handleDisable}
                  disabled={isLoading}
                  loading={disableService.isPending}
                  className="justify-start"
                >
                  <PowerOff className="h-4 w-4 mr-2" />
                  Disable on Boot
                </Button>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>

      <Card>
        <CardHeader>
          <CardTitle>Service Information</CardTitle>
          <CardDescription>Understanding Docker service management</CardDescription>
        </CardHeader>
        <CardContent className="space-y-3">
          <div className="text-sm space-y-2">
            <div className="flex items-start space-x-2">
              <div className="w-2 h-2 rounded-full bg-blue-500 mt-2"></div>
              <div>
                <strong>Start:</strong> Starts the Docker daemon and makes it available for container operations
              </div>
            </div>
            <div className="flex items-start space-x-2">
              <div className="w-2 h-2 rounded-full bg-red-500 mt-2"></div>
              <div>
                <strong>Stop:</strong> Gracefully stops the Docker daemon and all running containers
              </div>
            </div>
            <div className="flex items-start space-x-2">
              <div className="w-2 h-2 rounded-full bg-yellow-500 mt-2"></div>
              <div>
                <strong>Restart:</strong> Stops and then starts the Docker service
              </div>
            </div>
            <div className="flex items-start space-x-2">
              <div className="w-2 h-2 rounded-full bg-green-500 mt-2"></div>
              <div>
                <strong>Enable on Boot:</strong> Configures Docker to start automatically when the system boots
              </div>
            </div>
            <div className="flex items-start space-x-2">
              <div className="w-2 h-2 rounded-full bg-gray-500 mt-2"></div>
              <div>
                <strong>Disable on Boot:</strong> Prevents Docker from starting automatically on system boot
              </div>
            </div>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}

export default ServiceManagement